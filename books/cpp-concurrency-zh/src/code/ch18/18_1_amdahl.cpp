// mode: run
// output: varies
// tsan: no（计时示例，重复次数多）
// timeout: 60
#include <chrono>
#include <cstdint>
#include <cstdio>
#include <thread>
#include <vector>

namespace {

volatile std::uint64_t sink;

std::uint64_t work(std::uint64_t begin, std::uint64_t end) {
    std::uint64_t sum = 0;
    for (std::uint64_t i = begin; i < end; ++i) {
        sum += (i ^ (i >> 7)) * 3 + 1;
    }
    return sum;
}

double run_mixed(std::uint64_t n, double serial_ratio, unsigned threads) {
    const std::uint64_t serial_n = static_cast<std::uint64_t>(n * serial_ratio);
    const std::uint64_t parallel_n = n - serial_n;

    const auto start = std::chrono::steady_clock::now();
    std::uint64_t total = work(0, serial_n);  // 串行段：主线程一个人做

    std::vector<std::uint64_t> partial(threads, 0);
    std::vector<std::thread> pool;
    pool.reserve(threads);
    for (unsigned t = 0; t < threads; ++t) {
        pool.emplace_back([&, t] {
            partial[t] = work(serial_n + parallel_n * t / threads,
                              serial_n + parallel_n * (t + 1) / threads);
        });
    }
    for (auto& th : pool) th.join();
    const auto stop = std::chrono::steady_clock::now();

    for (auto v : partial) total += v;
    sink = total;
    return std::chrono::duration<double, std::milli>(stop - start).count();
}

double best_of(std::uint64_t n, double ratio, unsigned threads, int repeats) {
    double best = 1e300;
    for (int r = 0; r < repeats; ++r) {
        const double ms = run_mixed(n, ratio, threads);
        if (ms < best) best = ms;
    }
    return best;
}

}  // namespace

int main() {
    constexpr std::uint64_t n = 60'000'000;
    constexpr int repeats = 2;

    const double serial = best_of(n, 0.0, 1, repeats);
    std::puts("serial,threads,ceiling,measured");
    for (double r : {0.05, 0.10, 0.20}) {
        for (unsigned p : {1u, 2u, 4u, 8u, 16u}) {
            const double ceiling = 1.0 / (r + (1.0 - r) / p);
            const double measured = serial / best_of(n, r, p, repeats);
            std::printf("%d,%u,%.3f,%.3f\n", static_cast<int>(r * 100), p,
                        ceiling, measured);
        }
    }
}
