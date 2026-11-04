// mode: run
// output: varies
// tsan: no（计时示例，线程数多）
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
        sum += (i ^ (i >> 6)) + 1;
    }
    return sum;
}

double run(std::uint64_t n, unsigned threads) {
    std::vector<std::uint64_t> partial(threads, 0);
    const auto start = std::chrono::steady_clock::now();
    std::vector<std::thread> pool;
    pool.reserve(threads);
    for (unsigned t = 0; t < threads; ++t) {
        pool.emplace_back([&, t] {
            partial[t] = work(n * t / threads, n * (t + 1) / threads);
        });
    }
    for (auto& th : pool) th.join();
    const auto stop = std::chrono::steady_clock::now();

    std::uint64_t total = 0;
    for (auto v : partial) total += v;
    sink = total;
    return std::chrono::duration<double, std::milli>(stop - start).count();
}

double best_of(std::uint64_t n, unsigned threads) {
    double best = 1e300;
    for (int r = 0; r < 2; ++r) {
        const double ms = run(n, threads);
        if (ms < best) best = ms;
    }
    return best;
}

}  // namespace

int main() {
    constexpr std::uint64_t n = 80'000'000;
    std::puts("threads,ms");
    for (unsigned p : {1u, 2u, 4u, 8u, 16u, 24u, 32u, 48u, 64u}) {
        std::printf("%u,%.3f\n", p, best_of(n, p));
    }
}
