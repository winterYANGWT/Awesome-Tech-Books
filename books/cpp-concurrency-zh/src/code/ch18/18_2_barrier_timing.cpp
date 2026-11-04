// mode: run
// output: varies
// tsan: no（计时示例，重复创建线程）
#include <array>
#include <chrono>
#include <cstdint>
#include <cstdio>
#include <latch>
#include <thread>
#include <vector>

namespace {

std::uint64_t work(std::uint64_t n) {
    std::uint64_t sum = 0;
    for (std::uint64_t i = 0; i < n; ++i) {
        sum += (i ^ (i >> 4)) + 1;
    }
    return sum;
}

double ms_since(std::chrono::steady_clock::time_point t0) {
    return std::chrono::duration<double, std::milli>(
               std::chrono::steady_clock::now() - t0)
        .count();
}

}  // namespace

int main() {
    constexpr int workers = 4;
    std::puts("run,main_ms,max_ms,best_max_ms");
    double best_max = 1e300;

    for (int run = 1; run <= 5; ++run) {
        std::latch start(workers + 1);
        std::array<double, workers> elapsed{};
        std::vector<std::thread> pool;
        pool.reserve(workers);

        for (int id = 0; id < workers; ++id) {
            pool.emplace_back([&, id] {
                start.arrive_and_wait();  // 所有线程在同一刻起跑
                const auto t0 = std::chrono::steady_clock::now();
                volatile std::uint64_t local_sink =
                    work(static_cast<std::uint64_t>(id + 1) * 8'000'000);
                (void)local_sink;
                elapsed[static_cast<std::size_t>(id)] = ms_since(t0);
            });
        }

        start.arrive_and_wait();
        const auto main_start = std::chrono::steady_clock::now();
        volatile std::uint64_t main_sink = work(4'000'000);
        (void)main_sink;
        const double main_ms = ms_since(main_start);

        for (auto& th : pool) th.join();

        double max_ms = main_ms;
        for (double v : elapsed) {
            if (v > max_ms) max_ms = v;
        }
        if (max_ms < best_max) best_max = max_ms;
        std::printf("%d,%.3f,%.3f,%.3f\n", run, main_ms, max_ms, best_max);
    }
}
