// mode: run
// output: varies
// tsan: no（计时示例，TSan 会改变指令开销）
#include <atomic>
#include <chrono>
#include <cstdio>
#include <vector>
#include <algorithm>

using Clock = std::chrono::steady_clock;
constexpr int kOps = 10'000'000;
constexpr int kRounds = 3;

template <typename F>
double median_ns(F&& f) {
    std::vector<double> samples;
    for (int i = 0; i < kRounds; ++i) {
        auto t0 = Clock::now();
        f();
        auto t1 = Clock::now();
        samples.push_back(
            std::chrono::duration<double, std::nano>(t1 - t0).count() / kOps);
    }
    std::sort(samples.begin(), samples.end());
    return samples[samples.size() / 2];
}

volatile long sink = 0;

template <std::memory_order Order>
double rmw_ns() {
    return median_ns([] {
        std::atomic<long> a{0};
        for (int i = 0; i < kOps; ++i) {
            a.fetch_add(1, Order);
        }
        sink = a.load(std::memory_order_relaxed);
    });
}

template <std::memory_order Order>
double store_ns() {
    return median_ns([] {
        std::atomic<long> a{0};
        for (int i = 0; i < kOps; ++i) {
            a.store(i, Order);
        }
        sink = a.load(std::memory_order_relaxed);
    });
}

int main() {
    // 预热，避免第一次的页错误和频率变化混进结果。
    rmw_ns<std::memory_order_relaxed>();
    store_ns<std::memory_order_relaxed>();

    std::printf("order,rmw_ns,store_ns\n");
    std::printf("relaxed,%.3f,%.3f\n",
                rmw_ns<std::memory_order_relaxed>(),
                store_ns<std::memory_order_relaxed>());
    std::printf("acq_rel,%.3f,%.3f\n",
                rmw_ns<std::memory_order_acq_rel>(),
                store_ns<std::memory_order_release>());
    std::printf("seq_cst,%.3f,%.3f\n",
                rmw_ns<std::memory_order_seq_cst>(),
                store_ns<std::memory_order_seq_cst>());
}
