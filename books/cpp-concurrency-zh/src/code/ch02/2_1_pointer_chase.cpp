// mode: run
// output: varies
// tsan: no（计时示例，TSan 下慢且数字没有意义）
// timeout: 30
// 64 MiB 的下标数组：顺序求和 vs. 沿随机置换跳着走。
#include <algorithm>
#include <chrono>
#include <cstdint>
#include <cstdio>
#include <numeric>
#include <random>
#include <vector>

namespace {

constexpr std::size_t kElements = 8u * 1024u * 1024u;  // 8M × 8 B = 64 MiB

using Clock = std::chrono::steady_clock;

double median_ms(std::vector<double> values) {
    std::sort(values.begin(), values.end());
    return values[values.size() / 2];
}

// ANCHOR: both
// 连续访问：每条 cache line 的 8 个下标都被用到。
double time_sequential(const std::vector<std::uint64_t>& next, volatile std::uint64_t& sink) {
    {   // 预热：把页表和分支预测器先跑热，不记入样本
        std::uint64_t warm = 0;
        for (std::size_t i = 0; i < next.size(); ++i) {
            warm += next[i];
        }
        sink += warm;
    }
    std::vector<double> samples;
    for (int rep = 0; rep < 5; ++rep) {
        const auto start = Clock::now();
        std::uint64_t sum = 0;
        for (std::size_t i = 0; i < next.size(); ++i) {
            sum += next[i];
        }
        const auto stop = Clock::now();
        sink += sum;
        samples.push_back(std::chrono::duration<double, std::milli>(stop - start).count());
    }
    return median_ms(samples);
}

// 依赖式随机访问：下一次读哪里由这次读到的值决定，CPU 无法预取。
double time_random(const std::vector<std::uint64_t>& next, volatile std::uint64_t& sink) {
    std::uint64_t cur = 0;
    {   // 预热：先走一遍，避免第一次的页表缺失算进样本
        std::uint64_t warm = 0;
        for (std::size_t step = 0; step < next.size(); ++step) {
            cur = next[cur];
            warm += cur;
        }
        sink += warm;
    }
    std::vector<double> samples;
    for (int rep = 0; rep < 3; ++rep) {
        const auto start = Clock::now();
        std::uint64_t sum = 0;
        for (std::size_t step = 0; step < next.size(); ++step) {
            cur = next[cur];
            sum += cur;
        }
        const auto stop = Clock::now();
        sink += sum;
        samples.push_back(std::chrono::duration<double, std::milli>(stop - start).count());
    }
    return median_ms(samples);
}
// ANCHOR_END: both

}  // namespace

int main() {
    std::vector<std::uint64_t> next(kElements);
    std::iota(next.begin(), next.end(), 0);
    std::mt19937_64 rng(42);
    std::shuffle(next.begin(), next.end(), rng);

    volatile std::uint64_t sink = 0;
    const double sequential_ms = time_sequential(next, sink);
    const double random_ms = time_random(next, sink);

    std::printf("mode,ns_per_element\n");
    std::printf("sequential,%.3f\n", sequential_ms * 1e6 / kElements);
    std::printf("random,%.2f\n", random_ms * 1e6 / kElements);
    return 0;
}
