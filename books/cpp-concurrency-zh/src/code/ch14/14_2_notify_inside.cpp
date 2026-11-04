// mode: run
// output: varies
// expect: notify,ns_per_op
// tsan: no（计时示例，TSan 下会慢到无法比较）
// 同一个队列，只改 notify_one 的位置：锁内还是解锁之后。
#include <algorithm>
#include <chrono>
#include <condition_variable>
#include <cstdint>
#include <cstdio>
#include <mutex>
#include <optional>
#include <queue>
#include <thread>
#include <utility>
#include <vector>

namespace {

class Queue {
public:
    void push(int value, bool notify_inside) {
        std::unique_lock<std::mutex> lock(m_);
        queue_.push(value);
        if (notify_inside) {
            cv_.notify_one();  // 消费者醒来后先卡在 m_ 上
        }
        lock.unlock();
        if (!notify_inside) {
            cv_.notify_one();  // 已经解锁，消费者可以直接取
        }
    }

    int wait_pop() {
        std::unique_lock<std::mutex> lock(m_);
        cv_.wait(lock, [&] { return !queue_.empty(); });
        const int value = queue_.front();
        queue_.pop();
        return value;
    }

private:
    std::mutex m_;
    std::condition_variable cv_;
    std::queue<int> queue_;
};

std::int64_t run_once(bool notify_inside) {
    constexpr int kItems = 50000;
    Queue queue;
    const auto start = std::chrono::steady_clock::now();

    std::thread consumer([&] {
        std::int64_t sum = 0;
        for (int i = 0; i < kItems; ++i) {
            sum += queue.wait_pop();
        }
        if (sum != std::int64_t{kItems} * (kItems + 1) / 2) {
            std::printf("校验和错误\n");
        }
    });
    std::thread producer([&] {
        for (int i = 1; i <= kItems; ++i) {
            queue.push(i, notify_inside);
        }
    });

    producer.join();
    consumer.join();
    const auto stop = std::chrono::steady_clock::now();
    const auto ns = std::chrono::duration_cast<std::chrono::nanoseconds>(stop - start).count();
    return ns / kItems;
}

std::int64_t median(std::vector<std::int64_t> values) {
    std::sort(values.begin(), values.end());
    return values[values.size() / 2];
}

}  // namespace

int main() {
    std::vector<std::int64_t> inside;
    std::vector<std::int64_t> outside;
    for (int rep = 0; rep < 5; ++rep) {
        inside.push_back(run_once(true));
        outside.push_back(run_once(false));
    }

    std::printf("notify,ns_per_op\n");
    std::printf("inside,%lld\n", static_cast<long long>(median(inside)));
    std::printf("outside,%lld\n", static_cast<long long>(median(outside)));
    return 0;
}
