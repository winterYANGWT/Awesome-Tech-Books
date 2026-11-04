// mode: run
// output: varies
// expect: 每次结果都是 2000000
#include <algorithm>
#include <atomic>
#include <chrono>
#include <cstdint>
#include <iostream>
#include <mutex>
#include <thread>
#include <vector>

constexpr int kThreads = 2;
constexpr int kIncrements = 1'000'000;
constexpr int kRounds = 10;

using Clock = std::chrono::steady_clock;

double median_ms(std::vector<double> samples) {
    std::sort(samples.begin(), samples.end());
    return samples[samples.size() / 2];
}

int main() {
    std::mutex m;
    std::uint64_t guarded = 0;
    std::atomic<std::uint64_t> atomic_counter{0};
    std::vector<double> mutex_ms;
    std::vector<double> atomic_ms;
    bool all_correct = true;

    for (int round = 0; round < kRounds; ++round) {
        guarded = 0;
        const auto t0 = Clock::now();
        std::thread a([&] {
            for (int i = 0; i < kIncrements; ++i) {
                std::lock_guard<std::mutex> lock(m);  // 构造加锁，析构解锁
                ++guarded;
            }
        });
        std::thread b([&] {
            for (int i = 0; i < kIncrements; ++i) {
                std::lock_guard<std::mutex> lock(m);
                ++guarded;
            }
        });
        a.join();
        b.join();
        mutex_ms.push_back(std::chrono::duration<double, std::milli>(Clock::now() - t0).count());
        all_correct = all_correct && (guarded == std::uint64_t(kThreads) * kIncrements);

        atomic_counter = 0;
        const auto t1 = Clock::now();
        std::thread c([&] {
            for (int i = 0; i < kIncrements; ++i) {
                atomic_counter.fetch_add(1, std::memory_order_relaxed);
            }
        });
        std::thread d([&] {
            for (int i = 0; i < kIncrements; ++i) {
                atomic_counter.fetch_add(1, std::memory_order_relaxed);
            }
        });
        c.join();
        d.join();
        atomic_ms.push_back(std::chrono::duration<double, std::milli>(Clock::now() - t1).count());
        all_correct = all_correct && (atomic_counter.load() == std::uint64_t(kThreads) * kIncrements);
    }

    const std::uint64_t expected = std::uint64_t(kThreads) * kIncrements;
    std::cout << kThreads << " 个线程各加 " << kIncrements << " 次，重复 " << kRounds << " 次\n";
    std::cout << "互斥量：中位耗时 " << median_ms(mutex_ms) << " ms\n";
    std::cout << "原子量：中位耗时 " << median_ms(atomic_ms) << " ms\n";
    std::cout << (all_correct ? "每次结果都是 " : "有结果不是 ") << expected << '\n';
}
