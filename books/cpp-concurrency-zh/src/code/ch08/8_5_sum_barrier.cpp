// mode: run
// output: varies
// expect: 总和正确: 是
// 两阶段 sum：第一阶段各线程算 partials[i]，屏障后 0 号线程合并，再一个屏障一起结束。
#include <barrier>

#include <chrono>
#include <cstdint>
#include <cstdio>
#include <thread>
#include <vector>

constexpr int kThreads = 4;
constexpr uint64_t kN = 100000000;  // 1..N 求和
using Clock = std::chrono::steady_clock;

uint64_t partials[kThreads];
uint64_t total = 0;
std::barrier phase_barrier{kThreads};
long long phase1_ns = 0;
long long phase2_ns = 0;

int main() {
    auto start = Clock::now();
    std::vector<std::thread> threads;
    for (int i = 0; i < kThreads; ++i) {
        threads.emplace_back([i, start] {
            uint64_t lo = kN / kThreads * i + 1;
            uint64_t hi = kN / kThreads * (i + 1);
            uint64_t sum = 0;
            for (uint64_t x = lo; x <= hi; ++x) sum += x;
            partials[i] = sum;  // 只有线程 i 写这一格，不需要锁

            phase_barrier.arrive_and_wait();  // 第一阶段结束：partials 全部写好
            if (i == 0) {
                auto t = Clock::now();
                phase1_ns = std::chrono::duration_cast<std::chrono::nanoseconds>(t - start).count();
                uint64_t merged = 0;
                for (uint64_t v : partials) merged += v;
                total = merged;
                phase2_ns = std::chrono::duration_cast<std::chrono::nanoseconds>(
                                Clock::now() - t)
                                .count();
            }
            phase_barrier.arrive_and_wait();  // 第二阶段结束：total 已经写好
        });
    }
    for (auto& t : threads) t.join();

    uint64_t serial = kN * (kN + 1) / 2;
    std::printf("总和 = %llu，串行结果 = %llu\n", (unsigned long long)total,
                (unsigned long long)serial);
    std::printf("总和正确: %s\n", total == serial ? "是" : "否");
    std::printf("第一阶段 %.1f ms，第二阶段 %.1f us\n", phase1_ns / 1e6,
                phase2_ns / 1e3);
}
