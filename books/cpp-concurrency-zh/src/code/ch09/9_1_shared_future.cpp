// mode: run
// expect: 计算次数: 1
// expect: 4 个线程都拿到同一结果: 是
// expect: 结果: 订单 42：已支付
// 一个 shared_future 复制成 4 份，每个线程拿自己那份 get()。
#include <array>
#include <atomic>
#include <chrono>
#include <cstdio>
#include <future>
#include <string>
#include <thread>

using namespace std::chrono_literals;

int main() {
    std::atomic<int> computations{0};
    std::future<std::string> once = std::async(std::launch::async, [&] {
        computations.fetch_add(1, std::memory_order_relaxed);
        std::this_thread::sleep_for(30ms);
        return std::string("订单 42：已支付");
    });
    const std::shared_future<std::string> shared = once.share();

    std::array<std::string, 4> got;
    std::array<std::thread, 4> readers;
    for (int i = 0; i < 4; ++i) {
        readers[i] = std::thread([shared, &got, i] {
            got[static_cast<size_t>(i)] = shared.get();
        });
    }
    for (std::thread& t : readers) {
        t.join();
    }

    bool same = true;
    for (const std::string& s : got) {
        same = same && s == got[0];
    }
    std::printf("计算次数: %d\n", computations.load());
    std::printf("4 个线程都拿到同一结果: %s\n", same ? "是" : "否");
    std::printf("结果: %s\n", got[0].c_str());
}
