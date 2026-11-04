// mode: run
// output: varies
// tsan: no（计时示例，重复测量总耗时）
// expect: ^阶段数,std::barrier,忙等
// 固定每阶段的计算量，改变阶段数，比较 std::barrier 与“互斥量 + 自旋”屏障的开销。
#include <barrier>

#include <algorithm>
#include <atomic>
#include <chrono>
#include <cstdint>
#include <cstdio>
#include <mutex>
#include <thread>
#include <vector>

constexpr int kThreads = 4;
constexpr int kPhases[] = {1, 10, 100, 1000};
constexpr int kIterations = 2000;  // 每阶段固定计算量
using Clock = std::chrono::steady_clock;

uint64_t sink[kThreads];  // 每个线程写自己的一格，防止计算被优化掉

// 忙等屏障：互斥量保护“计数 + 代”，等待时自旋。可复用。
class BusyBarrier {
public:
    explicit BusyBarrier(int n) : n_(n) {}
    void arrive_and_wait() {
        unsigned generation;
        int arrived;
        {
            std::lock_guard<std::mutex> lk(mutex_);
            generation = generation_.load(std::memory_order_relaxed);
            arrived = ++count_;
            if (arrived == n_) {
                count_ = 0;
                generation_.fetch_add(1, std::memory_order_release);
            }
        }
        if (arrived != n_) {
            while (generation_.load(std::memory_order_acquire) == generation) {
                std::this_thread::yield();
            }
        }
    }

private:
    int n_;
    std::mutex mutex_;
    int count_ = 0;
    std::atomic<unsigned> generation_{0};
};

template <class Barrier>
double run(Barrier& barrier, int phases) {
    auto start = Clock::now();
    std::vector<std::thread> threads;
    for (int t = 0; t < kThreads; ++t) {
        threads.emplace_back([&barrier, phases, t] {
            uint64_t acc = 0;
            for (int p = 0; p < phases; ++p) {
                for (int k = 0; k < kIterations; ++k) acc = acc * 6364136223846793005ULL + k;
                barrier.arrive_and_wait();
            }
            sink[t] = acc;
        });
    }
    for (auto& th : threads) th.join();
    return std::chrono::duration<double, std::milli>(Clock::now() - start).count();
}

int main() {
    std::printf("阶段数,std::barrier,忙等\n");
    for (int phases : kPhases) {
        double best_std = 1e9;
        double best_busy = 1e9;
        for (int repeat = 0; repeat < 3; ++repeat) {
            std::barrier<> standard(kThreads);  // 没有完成函数
            BusyBarrier busy(kThreads);
            best_std = std::min(best_std, run(standard, phases));
            best_busy = std::min(best_busy, run(busy, phases));
        }
        std::printf("%d,%.3f,%.3f\n", phases, best_std, best_busy);
    }
}
