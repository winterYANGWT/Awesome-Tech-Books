// mode: run
// output: varies
// expect: 结果不等于 2000000 的有 [1-9]\d* 次
// 用 relaxed 原子读、加、写，保留“读—改—写”被拆开的竞争，
// 但让程序本身没有数据竞争，可以把丢更新的次数统计出来。
#include <atomic>
#include <cstdint>
#include <iostream>
#include <thread>

int main() {
    constexpr int kThreads = 2;
    constexpr int kIncrements = 1'000'000;
    constexpr int kRounds = 20;
    int lost = 0;
    std::uint64_t worst = 2'000'000;

    for (int round = 0; round < kRounds; ++round) {
        // ANCHOR: add
        std::atomic<std::uint64_t> counter{0};
        auto add = [&] {
            for (int i = 0; i < kIncrements; ++i) {
                const std::uint64_t old = counter.load(std::memory_order_relaxed);
                counter.store(old + 1, std::memory_order_relaxed);  // 两次操作之间可以插进另一个线程
            }
        };
        std::thread a(add);
        std::thread b(add);
        a.join();
        b.join();

        // ANCHOR_END: add
        const std::uint64_t got = counter.load(std::memory_order_relaxed);
        if (got != kThreads * kIncrements) {
            ++lost;
            if (got < worst) {
                worst = got;
            }
        }
    }

    std::cout << "每次 " << kThreads << " 个线程各加 " << kIncrements << " 次，共 " << kRounds << " 次运行\n";
    std::cout << "结果不等于 2000000 的有 " << lost << " 次\n";
    if (lost > 0) {
        std::cout << "最少的一次只加到 " << worst << '\n';
    }
}
