// mode: run
// output: varies
// tsan: no（计时示例，TSan 下慢且数字没有意义）
// timeout: 30
// 两个线程各加自己的计数器：先让两个计数器相邻，再让它们各占一条 cache line。
#include <algorithm>
#include <atomic>
#include <chrono>
#include <cstdio>
#include <thread>
#include <vector>

#include <pthread.h>
#include <sched.h>

namespace {

constexpr long kIters = 10'000'000;
using Clock = std::chrono::steady_clock;

// ANCHOR: layouts
struct Adjacent {
    std::atomic<long> a{0};
    std::atomic<long> b{0};
};

struct Aligned {
    alignas(64) std::atomic<long> a{0};
    alignas(64) std::atomic<long> b{0};
};

// ANCHOR_END: layouts

void pin_to_cpu(int cpu) {
    cpu_set_t set;
    CPU_ZERO(&set);
    CPU_SET(cpu, &set);
    (void)pthread_setaffinity_np(pthread_self(), sizeof(set), &set);
}

// ANCHOR: bench
template <typename Counters>
double run(Counters& c) {
    std::vector<double> samples;
    for (int rep = 0; rep < 3; ++rep) {
        c.a.store(0, std::memory_order_relaxed);
        c.b.store(0, std::memory_order_relaxed);
        const auto start = Clock::now();
        std::thread t0([&] {
            pin_to_cpu(0);
            for (long i = 0; i < kIters; ++i) {
                c.a.store(c.a.load(std::memory_order_relaxed) + 1,
                          std::memory_order_relaxed);
            }
        });
        std::thread t1([&] {
            pin_to_cpu(1);  // 两个线程放在不同物理核上，排除同一核的干扰
            for (long i = 0; i < kIters; ++i) {
                c.b.store(c.b.load(std::memory_order_relaxed) + 1,
                          std::memory_order_relaxed);
            }
        });
        t0.join();
        t1.join();
        const auto stop = Clock::now();
        samples.push_back(std::chrono::duration<double, std::milli>(stop - start).count());
    }
    std::sort(samples.begin(), samples.end());
    return samples[samples.size() / 2];
}
// ANCHOR_END: bench

}  // namespace

int main() {
    Adjacent adjacent;
    Aligned aligned;
    const double adjacent_ms = run(adjacent);
    const double aligned_ms = run(aligned);

    std::printf("layout,time_ms,sum0,sum1\n");
    std::printf("adjacent,%.3f,%ld,%ld\n", adjacent_ms,
                adjacent.a.load(std::memory_order_relaxed),
                adjacent.b.load(std::memory_order_relaxed));
    std::printf("aligned,%.3f,%ld,%ld\n", aligned_ms,
                aligned.a.load(std::memory_order_relaxed),
                aligned.b.load(std::memory_order_relaxed));
    return 0;
}
