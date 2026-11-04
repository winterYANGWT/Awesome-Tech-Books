// mode: run
// output: varies
// tsan: no（计时示例，TSan 会改变指令开销）
// timeout: 60
#include <algorithm>
#include <atomic>
#include <chrono>
#include <cstdio>
#include <mutex>
#include <thread>
#include <vector>

class SpinLock {
public:
    void lock() {
        while (flag_.test_and_set(std::memory_order_acquire)) {
        }
    }
    void unlock() { flag_.clear(std::memory_order_release); }

private:
    std::atomic_flag flag_{};
};

using Clock = std::chrono::steady_clock;

template <typename Lock>
double measure(int nthreads, int ops_per_thread, Lock& lock, long& counter) {
    auto start = Clock::now();
    std::vector<std::thread> threads;
    threads.reserve(nthreads);
    for (int t = 0; t < nthreads; ++t) {
        threads.emplace_back([&] {
            for (int i = 0; i < ops_per_thread; ++i) {
                std::lock_guard<Lock> guard(lock);
                ++counter;
            }
        });
    }
    for (auto& th : threads) {
        th.join();
    }
    auto stop = Clock::now();
    double ns = std::chrono::duration<double, std::nano>(stop - start).count();
    return ns / (static_cast<double>(nthreads) * ops_per_thread);
}

template <typename Lock>
double median_of(int nthreads, int ops, Lock& lock, long& counter) {
    std::vector<double> samples;
    for (int r = 0; r < 3; ++r) {
        samples.push_back(measure(nthreads, ops, lock, counter));
    }
    std::sort(samples.begin(), samples.end());
    return samples[samples.size() / 2];
}

int main() {
    SpinLock spin;
    std::mutex mutex;
    long counter = 0;

    // 预热：让页表、频率和线程创建的开销先付掉。
    measure(8, 2'000, spin, counter);
    measure(8, 2'000, mutex, counter);

    const int ops = 20'000;
    std::printf("threads,spin_ns_per_op,mutex_ns_per_op\n");
    for (int n : {1, 2, 4, 8, 16, 32, 64}) {
        double s = median_of(n, ops, spin, counter);
        double m = median_of(n, ops, mutex, counter);
        std::printf("%d,%.1f,%.1f\n", n, s, m);
    }
    (void)counter;
}
