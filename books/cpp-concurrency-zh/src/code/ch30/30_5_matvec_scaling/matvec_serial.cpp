// 串行版：与 MPI 版用同样的行主序矩阵和同样的计算顺序，测出 T1。
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <vector>

int main(int argc, char** argv) {
    const int n = argc > 1 ? std::atoi(argv[1]) : 8192;
    const int reps = 5;

    std::vector<double> a(static_cast<std::size_t>(n) * n);
    std::vector<double> x(static_cast<std::size_t>(n), 1.0);
    std::vector<double> y(static_cast<std::size_t>(n), 0.0);
    for (int i = 0; i < n; ++i) {
        for (int j = 0; j < n; ++j) {
            a[static_cast<std::size_t>(i) * n + j] =
                static_cast<double>((i + j) % 7) * 0.5;
        }
    }

    double best_ms = 1e100;
    double checksum = 0.0;
    for (int rep = 0; rep < reps; ++rep) {
        const auto t0 = std::chrono::steady_clock::now();
        for (int i = 0; i < n; ++i) {
            double sum = 0.0;
            for (int j = 0; j < n; ++j) {
                sum += a[static_cast<std::size_t>(i) * n + j] * x[j];
            }
            y[static_cast<std::size_t>(i)] = sum;
        }
        const auto t1 = std::chrono::steady_clock::now();
        const double ms =
            std::chrono::duration<double, std::milli>(t1 - t0).count();
        if (ms < best_ms) {
            best_ms = ms;
        }
    }
    for (double v : y) {
        checksum += v;
    }
    std::fprintf(stderr, "serial checksum=%.3f\n", checksum);
    std::printf("%.3f\n", best_ms);
    return 0;
}
