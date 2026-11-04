// mode: run
// output: varies
// tsan: no（计时示例，TSan 下慢且数字没有意义）
// timeout: 30
// 同一个 4096×4096 矩阵，按行累加与按列累加的对比。
#include <algorithm>
#include <chrono>
#include <cstdint>
#include <cstdio>
#include <vector>

namespace {

constexpr std::size_t kN = 4096;
using Clock = std::chrono::steady_clock;

double median_ms(std::vector<double> values) {
    std::sort(values.begin(), values.end());
    return values[values.size() / 2];
}

// ANCHOR: loops
std::uint64_t sum_rows(const std::vector<std::uint32_t>& a) {
    std::uint64_t sum = 0;
    for (std::size_t i = 0; i < kN; ++i) {
        for (std::size_t j = 0; j < kN; ++j) {
            sum += a[i * kN + j];
        }
    }
    return sum;
}

std::uint64_t sum_cols(const std::vector<std::uint32_t>& a) {
    std::uint64_t sum = 0;
    for (std::size_t j = 0; j < kN; ++j) {
        for (std::size_t i = 0; i < kN; ++i) {
            sum += a[i * kN + j];
        }
    }
    return sum;
}
// ANCHOR_END: loops

template <typename F>
double time_it(F f, const std::vector<std::uint32_t>& a, volatile std::uint64_t& sink) {
    sink += f(a);  // 预热一次，不记入样本
    std::vector<double> samples;
    for (int rep = 0; rep < 3; ++rep) {
        const auto start = Clock::now();
        const std::uint64_t sum = f(a);
        const auto stop = Clock::now();
        sink += sum;
        samples.push_back(std::chrono::duration<double, std::milli>(stop - start).count());
    }
    return median_ms(samples);
}

}  // namespace

int main() {
    std::vector<std::uint32_t> a(kN * kN);
    for (std::size_t i = 0; i < a.size(); ++i) {
        a[i] = static_cast<std::uint32_t>((i * 2654435761u) >> 16);
    }

    volatile std::uint64_t sink = 0;
    const double rows_ms = time_it(sum_rows, a, sink);
    const double cols_ms = time_it(sum_cols, a, sink);

    std::printf("row_sum=%llu\n", static_cast<unsigned long long>(sum_rows(a)));
    std::printf("col_sum=%llu\n", static_cast<unsigned long long>(sum_cols(a)));
    std::printf("same_sum=%s\n", sum_rows(a) == sum_cols(a) ? "yes" : "no");
    std::printf("row_ms=%.3f\n", rows_ms);
    std::printf("col_ms=%.3f\n", cols_ms);
    std::printf("col_over_row=%.1f\n", cols_ms / rows_ms);
    return 0;
}
