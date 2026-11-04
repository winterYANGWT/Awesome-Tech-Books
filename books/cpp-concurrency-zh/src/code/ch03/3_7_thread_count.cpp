// mode: run
// output: varies
// tsan: no（计时示例，TSan 下耗时过长）
// timeout: 60
// expect: 块大小,线程数,耗时_ms
// 固定 N，改变块大小：块小则领块次数多，块大则并行度下降。
#include <algorithm>
#include <atomic>
#include <chrono>
#include <cstdint>
#include <cstdio>
#include <thread>
#include <vector>

namespace {

constexpr std::uint64_t kN = 100'000'000;

struct Result {
    std::uint64_t sum;
    int threads;
    double milliseconds;
};

Result dynamic_sum(std::uint64_t block_size, int max_threads) {
    const std::uint64_t blocks = (kN + block_size - 1) / block_size;
    const int threads = static_cast<int>(
        std::min<std::uint64_t>(blocks, static_cast<std::uint64_t>(max_threads)));

    std::atomic<std::uint64_t> next{1};
    std::vector<std::uint64_t> partials(static_cast<std::size_t>(threads), 0);
    std::vector<std::thread> workers;
    workers.reserve(static_cast<std::size_t>(threads));

    const auto start = std::chrono::steady_clock::now();
    for (int t = 0; t < threads; ++t) {
        workers.emplace_back([&next, &partials, t, block_size] {
            std::uint64_t local = 0;
            for (;;) {
                // 领下一块；内存序第 12 章再讲，这里只需要原子地取走一个块号。
                const std::uint64_t begin =
                    next.fetch_add(block_size, std::memory_order_relaxed);
                if (begin > kN) {
                    break;
                }
                const std::uint64_t end = std::min(kN + 1, begin + block_size);
                for (std::uint64_t i = begin; i < end; ++i) {
                    local += i;
                }
            }
            partials[static_cast<std::size_t>(t)] = local;
        });
    }
    for (auto& worker : workers) {
        worker.join();
    }

    std::uint64_t total = 0;
    for (std::uint64_t part : partials) {
        total += part;
    }
    const auto stop = std::chrono::steady_clock::now();
    return {total, threads,
            std::chrono::duration<double, std::milli>(stop - start).count()};
}

double median_ms(const std::vector<double>& values) {
    std::vector<double> sorted = values;
    std::sort(sorted.begin(), sorted.end());
    return sorted[sorted.size() / 2];
}

}  // namespace

int main() {
    unsigned hardware = std::thread::hardware_concurrency();
    int max_threads = hardware == 0 ? 4 : static_cast<int>(hardware);
    std::printf("hardware_concurrency() = %u（取不到时用 4）\n", hardware);

    const std::uint64_t expected = kN * (kN + 1) / 2;
    bool all_ok = true;
    struct Row {
        std::uint64_t block;
        int threads;
        double ms;
    };
    std::vector<Row> rows;

    {
        Result warmup = dynamic_sum(1 << 20, max_threads);
        all_ok = all_ok && warmup.sum == expected;
    }
    for (std::uint64_t block : {std::uint64_t{1} << 10, std::uint64_t{1} << 16,
                                std::uint64_t{1} << 20, std::uint64_t{1} << 24}) {
        std::vector<double> times;
        Result result{0, 0, 0};
        for (int repeat = 0; repeat < 3; ++repeat) {
            result = dynamic_sum(block, max_threads);
            times.push_back(result.milliseconds);
        }
        all_ok = all_ok && result.sum == expected;
        rows.push_back({block, result.threads, median_ms(times)});
    }

    std::printf("检查和等于 N(N+1)/2：%s\n", all_ok ? "是" : "否");
    std::printf("块大小,线程数,耗时_ms\n");
    for (const Row& row : rows) {
        std::printf("%llu,%d,%.1f\n", (unsigned long long)row.block, row.threads, row.ms);
    }
    return all_ok ? 0 : 1;
}
