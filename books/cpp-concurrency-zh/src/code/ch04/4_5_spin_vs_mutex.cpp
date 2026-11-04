// mode: run
// output: varies
// tsan: no（自旋锁计时示例，TSan 下 32 线程过慢）
// expect: ^32,
// 自旋锁和 std::mutex 保护同一个短临界区，线程数从 1 加到 32。
// 输出 CSV，图 4-x 直接读它。
#include <algorithm>
#include <atomic>
#include <chrono>
#include <cstdint>
#include <iomanip>
#include <iostream>
#include <mutex>
#include <thread>
#include <vector>

using Clock = std::chrono::steady_clock;
constexpr int kIters = 50'000;
constexpr int kRepeats = 3;
const std::vector<int> kThreadCounts{1, 2, 4, 8, 16, 32};

class SpinLock {
public:
    void lock() {
        while (flag_.test_and_set(std::memory_order_acquire)) {
#if defined(__x86_64__) || defined(__i386__)
            __builtin_ia32_pause();
#endif
        }
    }

    void unlock() { flag_.clear(std::memory_order_release); }

private:
    std::atomic_flag flag_ = ATOMIC_FLAG_INIT;
};

template <class Lock>
double measure(int thread_count, std::uint64_t& counter) {
    double best = 1e100;
    for (int repeat = 0; repeat < kRepeats; ++repeat) {
        Lock lock;
        counter = 0;
        const auto t0 = Clock::now();
        std::vector<std::thread> threads;
        threads.reserve(thread_count);
        for (int t = 0; t < thread_count; ++t) {
            threads.emplace_back([&] {
                for (int i = 0; i < kIters; ++i) {
                    std::lock_guard<Lock> guard(lock);
                    ++counter;
                }
            });
        }
        for (auto& thread : threads) {
            thread.join();
        }
        const double ns = std::chrono::duration<double, std::nano>(Clock::now() - t0).count();
        best = std::min(best, ns / (double(thread_count) * kIters));
    }
    return best;
}

int main() {
    std::uint64_t counter = 0;
    std::cout << "threads,spin_ns,mutex_ns\n" << std::fixed << std::setprecision(1);
    for (int threads : kThreadCounts) {
        const double spin = measure<SpinLock>(threads, counter);
        const double mutex = measure<std::mutex>(threads, counter);
        std::cout << threads << ',' << spin << ',' << mutex << '\n';
    }
}
