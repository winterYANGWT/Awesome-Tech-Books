// mode: run
// output: varies
// tsan: no（计时示例，重复次数多）
// timeout: 60
#include <atomic>
#include <chrono>
#include <cstdint>
#include <cstdio>
#include <thread>
#include <vector>

namespace {

volatile std::uint64_t sink;
constexpr unsigned threads = 8;
constexpr unsigned long n = 20'000'000;

std::uint64_t work(std::uint64_t begin, std::uint64_t end) {
    std::uint64_t sum = 0;
    for (std::uint64_t i = begin; i < end; ++i) {
        sum += (i ^ (i >> 3)) & 0xff;
    }
    return sum;
}

double ms_since(std::chrono::steady_clock::time_point t0) {
    return std::chrono::duration<double, std::milli>(
               std::chrono::steady_clock::now() - t0)
        .count();
}

double local_accum() {
    std::vector<std::uint64_t> partial(threads, 0);
    const auto t0 = std::chrono::steady_clock::now();
    std::vector<std::thread> pool;
    pool.reserve(threads);
    for (unsigned t = 0; t < threads; ++t) {
        pool.emplace_back([&, t] {
            partial[t] = work(n * t / threads, n * (t + 1) / threads);
        });
    }
    for (auto& th : pool) th.join();
    std::uint64_t total = 0;
    for (auto v : partial) total += v;
    sink = total;
    return ms_since(t0);
}

double shared_atomic() {
    std::atomic<std::uint64_t> total{0};
    const auto t0 = std::chrono::steady_clock::now();
    std::vector<std::thread> pool;
    pool.reserve(threads);
    for (unsigned t = 0; t < threads; ++t) {
        pool.emplace_back([&, t] {
            const std::uint64_t begin = n * t / threads;
            const std::uint64_t end = n * (t + 1) / threads;
            for (std::uint64_t i = begin; i < end; ++i) {
                total.fetch_add((i ^ (i >> 3)) & 0xff,
                                std::memory_order_relaxed);
            }
        });
    }
    for (auto& th : pool) th.join();
    sink = total.load(std::memory_order_relaxed);
    return ms_since(t0);
}

}  // namespace

int main() {
    constexpr int repeats = 3;
    double best_local = 1e300;
    double best_shared = 1e300;
    for (int r = 0; r < repeats; ++r) {
        const double l = local_accum();
        const double s = shared_atomic();
        if (l < best_local) best_local = l;
        if (s < best_shared) best_shared = s;
    }
    std::puts("method,ms");
    std::printf("local,%.3f\n", best_local);
    std::printf("shared_atomic,%.3f\n", best_shared);
}
