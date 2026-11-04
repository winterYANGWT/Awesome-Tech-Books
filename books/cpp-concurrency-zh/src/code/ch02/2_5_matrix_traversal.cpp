// mode: run
// output: varies
// tsan: no（计时示例，TSan 下慢且数字没有意义）
// timeout: 60
// 同一个 4096×4096 矩阵，三种遍历顺序；比较耗时和校验和。
#include <algorithm>
#include <chrono>
#include <cstdint>
#include <cstdio>
#include <vector>

namespace {

constexpr std::size_t kN = 4096;
constexpr std::size_t kBlock = 4;
using Clock = std::chrono::steady_clock;

// ANCHOR: orders
std::uint64_t sum_ij(const std::vector<std::uint32_t>& a) {
    std::uint64_t sum = 0;
    for (std::size_t i = 0; i < kN; ++i) {
        for (std::size_t j = 0; j < kN; ++j) {
            sum += a[i * kN + j];
        }
    }
    return sum;
}

std::uint64_t sum_ji(const std::vector<std::uint32_t>& a) {
    std::uint64_t sum = 0;
    for (std::size_t j = 0; j < kN; ++j) {
        for (std::size_t i = 0; i < kN; ++i) {
            sum += a[i * kN + j];
        }
    }
    return sum;
}

std::uint64_t sum_blocked(const std::vector<std::uint32_t>& a) {
    std::uint64_t sum = 0;
    for (std::size_t ii = 0; ii < kN; ii += kBlock) {
        for (std::size_t jj = 0; jj < kN; jj += kBlock) {
            for (std::size_t i = ii; i < ii + kBlock; ++i) {
                for (std::size_t j = jj; j < jj + kBlock; ++j) {
                    sum += a[i * kN + j];
                }
            }
        }
    }
    return sum;
}
// ANCHOR_END: orders

template <typename F>
void measure(const char* name, F f, const std::vector<std::uint32_t>& a) {
    std::vector<double> samples;
    std::uint64_t sum = 0;
    for (int rep = 0; rep < 3; ++rep) {
        const auto start = Clock::now();
        sum = f(a);
        const auto stop = Clock::now();
        samples.push_back(std::chrono::duration<double, std::milli>(stop - start).count());
    }
    std::sort(samples.begin(), samples.end());
    std::printf("%s,%.3f,%llu\n", name, samples[1],
                static_cast<unsigned long long>(sum));
}

}  // namespace

int main() {
    std::vector<std::uint32_t> a(kN * kN);
    for (std::size_t i = 0; i < a.size(); ++i) {
        a[i] = static_cast<std::uint32_t>((i * 2654435761u) >> 16);
    }

    std::printf("order,time_ms,checksum\n");
    measure("i-j", sum_ij, a);
    measure("j-i", sum_ji, a);
    measure("block-4", sum_blocked, a);
    return 0;
}
