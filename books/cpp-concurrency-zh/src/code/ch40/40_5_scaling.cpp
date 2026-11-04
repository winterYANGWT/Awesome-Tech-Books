// mode: run
// tsan: no（计时示例）
// output: varies
// expect: threads,time_ms,speedup
// expect: 32,

#include <algorithm>
#include <chrono>
#include <cstdint>
#include <cstdio>
#include <thread>
#include <vector>

namespace {
volatile std::uint64_t g_sink = 0;
constexpr std::uint64_t kCount = 200'000'000;

double run_once(int threads) {
    std::vector<std::uint64_t> partial(static_cast<std::size_t>(threads), 0);
    std::vector<std::thread> pool;

    const auto start = std::chrono::steady_clock::now();
    for (int t = 0; t < threads; ++t) {
        pool.emplace_back([&, t] {
            const std::uint64_t begin = kCount * static_cast<std::uint64_t>(t) /
                                        static_cast<std::uint64_t>(threads);
            const std::uint64_t end = kCount * static_cast<std::uint64_t>(t + 1) /
                                      static_cast<std::uint64_t>(threads);
            std::uint64_t sum = 0;
            for (std::uint64_t i = begin; i < end; ++i) sum += i;
            partial[static_cast<std::size_t>(t)] = sum;
        });
    }
    for (auto& thread : pool) thread.join();
    const auto stop = std::chrono::steady_clock::now();

    std::uint64_t total = 0;
    for (std::uint64_t value : partial) total += value;
    g_sink = total;

    return std::chrono::duration<double, std::milli>(stop - start).count();
}
}  // namespace

int main() {
    const int counts[] = {1, 2, 4, 8, 16, 32};
    double baseline = 0.0;

    std::printf("threads,time_ms,speedup\n");
    for (int threads : counts) {
        std::vector<double> samples;
        for (int rep = 0; rep < 5; ++rep) samples.push_back(run_once(threads));
        std::sort(samples.begin(), samples.end());
        const double median = samples[samples.size() / 2];
        if (threads == 1) baseline = median;
        std::printf("%d,%.3f,%.3f\n", threads, median, baseline / median);
    }
}
