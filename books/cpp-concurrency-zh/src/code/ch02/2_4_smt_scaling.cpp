// mode: run
// output: varies
// tsan: no（计时示例，TSan 下慢且数字没有意义）
// timeout: 60
// 固定总量的整数运算，线程数 1..32；16 个物理核、每核 2 个 SMT 线程。
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include <algorithm>
#include <chrono>
#include <cstdint>
#include <cstdio>
#include <thread>
#include <vector>

#include <pthread.h>
#include <sched.h>

namespace {

constexpr std::uint64_t kTotal = 1'500'000'000;
using Clock = std::chrono::steady_clock;

void pin_to_cpu(int cpu) {
    cpu_set_t set;
    CPU_ZERO(&set);
    CPU_SET(cpu, &set);
    (void)pthread_setaffinity_np(pthread_self(), sizeof(set), &set);
}

// ANCHOR: work
std::uint64_t work(std::uint64_t count, std::uint64_t seed) {
    std::uint64_t x = seed | 1;
    std::uint64_t sum = 0;
    for (std::uint64_t i = 0; i < count; ++i) {
        x ^= x << 13;
        x ^= x >> 7;
        x ^= x << 17;
        sum += x;
    }
    return sum;
}
// ANCHOR_END: work

double run_once(int threads) {
    const std::uint64_t chunk = kTotal / static_cast<std::uint64_t>(threads);
    std::vector<std::uint64_t> sums(static_cast<std::size_t>(threads), 0);
    std::vector<std::thread> pool;
    pool.reserve(static_cast<std::size_t>(threads));

    const auto start = Clock::now();
    for (int t = 0; t < threads; ++t) {
        pool.emplace_back([&, t] {
            pin_to_cpu(t);
            sums[static_cast<std::size_t>(t)] =
                work(chunk, 0x9e3779b97f4a7c15ULL + static_cast<std::uint64_t>(t));
        });
    }
    for (auto& th : pool) {
        th.join();
    }
    const auto stop = Clock::now();

    std::uint64_t check = 0;
    for (std::uint64_t s : sums) {
        check += s;
    }
    if (check == 0) {
        std::fprintf(stderr, "unexpected zero checksum\n");
    }
    return std::chrono::duration<double, std::milli>(stop - start).count();
}

double best_of_two(int threads) {
    return std::min(run_once(threads), run_once(threads));
}

}  // namespace

int main() {
    pin_to_cpu(0);
    const double base = best_of_two(1);
    std::printf("threads,time_ms,speedup\n");
    for (int p : {1, 2, 4, 8, 16, 32}) {
        const double ms = p == 1 ? base : best_of_two(p);
        std::printf("%d,%.3f,%.2f\n", p, ms, base / ms);
    }
    return 0;
}
