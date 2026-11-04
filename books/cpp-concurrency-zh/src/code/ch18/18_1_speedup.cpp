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
        sum += (i ^ (i >> 5)) + 1;
    }
    return sum;
}

double run_parallel(std::uint64_t n, unsigned threads) {
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

double best_of(std::uint64_t n, unsigned threads, int repeats) {
    double best = 1e300;
    for (int r = 0; r < repeats; ++r) {
        const double ms = run_parallel(n, threads);
        if (ms < best) best = ms;
    }
    return best;
}

}  // namespace

int main() {
    constexpr std::uint64_t n = 120'000'000;
    constexpr int repeats = 3;
    const double serial = best_of(n, 1, repeats);

    std::puts("threads,speedup,ideal");
    for (unsigned p : {1u, 2u, 4u, 8u, 16u}) {
        const double parallel = best_of(n, p, repeats);
        const double speedup = serial / parallel;
        std::printf("%u,%.3f,%.3f\n", p, speedup, static_cast<double>(p));
    }
}
