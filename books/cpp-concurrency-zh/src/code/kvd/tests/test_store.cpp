#include "store.hpp"

#include <atomic>
#include <barrier>
#include <cstddef>
#include <iostream>
#include <mutex>
#include <optional>
#include <string>
#include <thread>
#include <vector>

namespace {

std::atomic<int> failures{0};

void expect(bool ok, const std::string& what) {
    if (ok) return;
    ++failures;
    static std::mutex mutex;
    std::lock_guard lock(mutex);  // 失败时别再让几行输出缠在一起
    std::cerr << "FAIL: " << what << '\n';
}

std::string thread_name(int i) { return "t" + std::to_string(i); }

bool known_shared_value(const std::string& value, int threads) {
    if (value == "init") return true;
    if (value.size() != 2 || value[0] != 't') return false;
    return value[1] >= '0' && value[1] < '0' + threads;
}

void test_single_thread() {
    Store store(4);
    expect(!store.get("missing").has_value(), "get missing");
    expect(!store.del("missing"), "del missing");
    expect(store.size() == 0, "empty size");

    store.set("a", "1");
    expect(store.get("a") == std::optional<std::string>("1"), "set then get");
    store.set("a", "2");
    expect(store.get("a") == std::optional<std::string>("2"), "overwrite");
    expect(store.size() == 1, "size after overwrite");

    expect(store.del("a"), "del existing");
    expect(!store.get("a").has_value(), "get after del");
    expect(!store.del("a"), "del twice");
    expect(store.size() == 0, "size after del");

    Request set;
    set.op = Request::Op::Set;
    set.key = "k";
    set.value = "v";
    Response response = store.apply(set);
    expect(response.kind == Response::Kind::Ok, "apply SET -> OK");

    Request get;
    get.op = Request::Op::Get;
    get.key = "k";
    response = store.apply(get);
    expect(response.kind == Response::Kind::Value && response.data == "v",
           "apply GET -> VALUE v");

    Request del;
    del.op = Request::Op::Del;
    del.key = "k";
    response = store.apply(del);
    expect(response.kind == Response::Kind::Deleted, "apply DEL -> DELETED");
    response = store.apply(del);
    expect(response.kind == Response::Kind::NotFound,
           "apply DEL missing -> NOT_FOUND");

    // 桶数只是分片参数，不影响语义。
    Store one_bucket(1);
    one_bucket.set("x", "y");
    expect(one_bucket.get("x") == std::optional<std::string>("y"),
           "one bucket");
    Store zero_bucket(0);  // 防御性处理：至少有一个桶
    zero_bucket.set("x", "y");
    expect(zero_bucket.get("x") == std::optional<std::string>("y"),
           "zero bucket count");
}

void test_concurrent() {
    constexpr int kThreads = 8;
    constexpr int kOwnedPerThread = 32;
    constexpr int kSharedKeys = 8;
    constexpr int kIterations = 500;

    Store store(16);
    for (int j = 0; j < kSharedKeys; ++j) {
        store.set("shared-" + std::to_string(j), "init");
    }

    std::barrier start(kThreads + 1);
    std::vector<std::thread> threads;
    threads.reserve(kThreads);
    for (int i = 0; i < kThreads; ++i) {
        threads.emplace_back([&store, &start, i] {
            start.arrive_and_wait();
            const std::string me = thread_name(i);

            // 每个线程写自己的 key：结束时的值必须是确定的。
            for (int k = 0; k < kOwnedPerThread; ++k) {
                const std::string key =
                    "own-" + me + "-" + std::to_string(k);
                const std::string value = "v-" + me + "-" + std::to_string(k);
                store.set(key, value);
                expect(store.get(key) == std::optional<std::string>(value),
                       "own key after set");
            }

            // 所有线程抢同一组 key：只检查读到的值是写过的某个值。
            for (int iter = 0; iter < kIterations; ++iter) {
                const std::string key =
                    "shared-" + std::to_string(iter % kSharedKeys);
                store.set(key, me);
                auto value = store.get(key);
                expect(value.has_value(), "shared key must exist");
                if (value) {
                    expect(known_shared_value(*value, kThreads),
                           "shared value is one that was written");
                }

                // 同一个 key 的 set/del 由本线程独占，结果必须确定。
                const std::string churn = "churn-" + std::to_string(i);
                store.set(churn, me);
                expect(store.del(churn), "del own churn key");

                // size() 可以在别人写的时候调用，只要求不崩、不报错。
                (void)store.size();
            }

            for (int k = 0; k < kOwnedPerThread; ++k) {
                store.set("own-" + me + "-" + std::to_string(k), "final");
            }
        });
    }

    start.arrive_and_wait();
    for (auto& thread : threads) thread.join();

    expect(failures.load() == 0, "no thread reported a failure");
    expect(store.size() == kSharedKeys + kThreads * kOwnedPerThread,
           "final size: shared + owned, churn keys deleted");

    for (int i = 0; i < kThreads; ++i) {
        const std::string me = thread_name(i);
        for (int k = 0; k < kOwnedPerThread; ++k) {
            const std::string key = "own-" + me + "-" + std::to_string(k);
            expect(store.get(key) == std::optional<std::string>("final"),
                   "final owned value");
        }
    }

    // 共享 key 的赢家不确定；主线程最后写一遍再检查，语义必须是确定的。
    for (int j = 0; j < kSharedKeys; ++j) {
        const std::string key = "shared-" + std::to_string(j);
        auto value = store.get(key);
        expect(value.has_value() && known_shared_value(*value, kThreads),
               "shared final value is known");
        store.set(key, "final");
        expect(store.get(key) == std::optional<std::string>("final"),
               "shared key after final set");
    }
}

}  // namespace

int main() {
    test_single_thread();
    test_concurrent();

    if (failures.load() != 0) {
        std::cerr << failures.load() << " failure(s)\n";
        return 1;
    }
    std::cout << "test_store: OK\n";
    return 0;
}
