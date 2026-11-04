// mode: run
// output: varies
// tsan: no（计时示例，数据量大）
// timeout: 90
#include <algorithm>
#include <chrono>
#include <cstdint>
#include <cstdio>
#include <latch>
#include <string>
#include <thread>
#include <vector>

namespace {

volatile double sink;

void compute(double* y, const double* A, const double* x, std::size_t n,
             std::size_t m, unsigned threads, unsigned tid) {
    const std::size_t begin = n * tid / threads;
    const std::size_t end = n * (tid + 1) / threads;
    volatile double* vy = y;  // 强制每次迭代都真正写回 y
    for (std::size_t i = begin; i < end; ++i) {
        for (std::size_t j = 0; j < m; ++j) {
            vy[i] += A[i * m + j] * x[j];
        }
    }
}

double run_shape(std::size_t n, std::size_t m, unsigned threads) {
    std::vector<double> A(n * m), x(m), y(n);
    for (std::size_t k = 0; k < A.size(); ++k) {
        A[k] = static_cast<double>((k * 17) % 13) * 0.001;
    }
    for (std::size_t j = 0; j < m; ++j) {
        x[j] = 0.5 + static_cast<double>(j % 7) * 0.01;
    }
    std::fill(y.begin(), y.end(), 0.0);

    std::latch ready(threads);
    std::latch go(1);
    std::vector<std::thread> pool;
    pool.reserve(threads);
    for (unsigned t = 0; t < threads; ++t) {
        pool.emplace_back([&, t] {
            ready.count_down();
            go.wait();
            compute(y.data(), A.data(), x.data(), n, m, threads, t);
        });
    }

    const auto start = std::chrono::steady_clock::now();
    ready.wait();
    go.count_down();
    for (auto& th : pool) th.join();
    const auto stop = std::chrono::steady_clock::now();

    sink = y[0] + y[n / 2] + y[n - 1];
    return std::chrono::duration<double, std::milli>(stop - start).count();
}

double best_of(std::size_t n, std::size_t m, unsigned threads) {
    double best = 1e300;
    for (int r = 0; r < 2; ++r) {
        const double ms = run_shape(n, m, threads);
        if (ms < best) best = ms;
    }
    return best;
}

}  // namespace

int main() {
    constexpr std::size_t tall_rows = 1'000'000;
    constexpr std::size_t square = 2828;
    constexpr std::size_t wide_cols = 1'000'000;

    std::puts("shape,threads,ms");
    for (unsigned p : {1u, 2u, 4u, 8u}) {
        std::printf("tall_thin,%u,%.3f\n", p, best_of(tall_rows, 8, p));
        std::printf("square,%u,%.3f\n", p, best_of(square, square, p));
        std::printf("short_wide,%u,%.3f\n", p, best_of(8, wide_cols, p));
    }
}
