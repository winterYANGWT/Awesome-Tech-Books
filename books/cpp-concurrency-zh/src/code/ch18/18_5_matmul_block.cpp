// mode: run
// output: varies
// tsan: no（计时示例，数据量大）
// timeout: 60
#include <algorithm>
#include <chrono>
#include <cstdint>
#include <cstdio>
#include <vector>

namespace {

volatile double sink;
constexpr std::size_t n = 400;

double run_block(std::size_t block) {
    std::vector<double> A(n * n), B(n * n), C(n * n, 0.0);
    for (std::size_t k = 0; k < n * n; ++k) {
        A[k] = static_cast<double>((k * 17) % 13) * 0.01;
        B[k] = static_cast<double>((k * 7) % 11) * 0.01;
    }

    const auto start = std::chrono::steady_clock::now();
    for (std::size_t ii = 0; ii < n; ii += block) {
        const std::size_t iend = std::min(ii + block, n);
        for (std::size_t kk = 0; kk < n; kk += block) {
            const std::size_t kend = std::min(kk + block, n);
            for (std::size_t jj = 0; jj < n; jj += block) {
                const std::size_t jend = std::min(jj + block, n);
                for (std::size_t i = ii; i < iend; ++i) {
                    for (std::size_t k = kk; k < kend; ++k) {
                        const double a = A[i * n + k];
                        for (std::size_t j = jj; j < jend; ++j) {
                            C[i * n + j] += a * B[k * n + j];
                        }
                    }
                }
            }
        }
    }
    const auto stop = std::chrono::steady_clock::now();
    sink = C[0] + C[n * n / 2] + C[n * n - 1];
    return std::chrono::duration<double, std::milli>(stop - start).count();
}

}  // namespace

int main() {
    std::puts("block,ms");
    for (std::size_t block : {8u, 16u, 32u, 64u, 128u, 256u, 400u}) {
        double best = 1e300;
        for (int r = 0; r < 2; ++r) {
            const double ms = run_block(block);
            if (ms < best) best = ms;
        }
        std::printf("%zu,%.3f\n", block, best);
    }
}
