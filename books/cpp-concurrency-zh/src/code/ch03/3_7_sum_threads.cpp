// mode: run
// output: varies
// tsan: no（计时示例，TSan 下耗时过长）
// timeout: 60
// expect: 检查和等于 N\(N\+1\)/2：是
// 把 1..N 按连续块分给 p 个线程，每个线程只写自己的 partials[i]，最后串行合并。
#include <algorithm>
#include <chrono>
#include <cstdint>
#include <cstdio>
#include <iterator>
#include <numeric>
#include <thread>
#include <vector>

namespace {

constexpr std::uint64_t kN = 100'000'000;

// 串行基线用 std::accumulate 遍历 1..N。编译器会把纯算术循环折成闭式，
// 所以迭代器每步读一个 volatile 步长，保证计时测到的是循环本身。
volatile std::uint64_t g_step = 1;

struct CountingIterator {
    using iterator_category = std::input_iterator_tag;
    using value_type = std::uint64_t;
    using difference_type = std::ptrdiff_t;
    using pointer = const std::uint64_t*;
    using reference = std::uint64_t;

    std::uint64_t value;

    std::uint64_t operator*() const { return value; }
    CountingIterator& operator++() {
        value += g_step;
        return *this;
    }
    bool operator!=(const CountingIterator& other) const { return value != other.value; }
};

std::uint64_t sum_range(std::uint64_t begin, std::uint64_t end) {
    std::uint64_t sum = 0;
    for (std::uint64_t i = begin; i < end; ++i) {
        sum += i;
    }
    return sum;
}

std::uint64_t parallel_sum(int threads, double& milliseconds) {
    std::vector<std::uint64_t> partials(static_cast<std::size_t>(threads), 0);
    std::vector<std::thread> workers;
    workers.reserve(static_cast<std::size_t>(threads));

    const std::uint64_t block = (kN + static_cast<std::uint64_t>(threads) - 1) /
                                static_cast<std::uint64_t>(threads);

    const auto start = std::chrono::steady_clock::now();
    for (int t = 0; t < threads; ++t) {
        const std::uint64_t begin = 1 + static_cast<std::uint64_t>(t) * block;
        const std::uint64_t end = std::min(kN + 1, begin + block);
        workers.emplace_back([&partials, t, begin, end] {
            partials[static_cast<std::size_t>(t)] = sum_range(begin, end);
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
    milliseconds = std::chrono::duration<double, std::milli>(stop - start).count();
    return total;
}

double median_ms(const std::vector<double>& values) {
    std::vector<double> sorted = values;
    std::sort(sorted.begin(), sorted.end());
    return sorted[sorted.size() / 2];
}

}  // namespace

int main() {
    const std::uint64_t expected = kN * (kN + 1) / 2;

    std::vector<double> serial_times;
    std::uint64_t serial_sum = 0;
    for (int repeat = 0; repeat < 3; ++repeat) {
        const auto start = std::chrono::steady_clock::now();
        serial_sum = std::accumulate(CountingIterator{1}, CountingIterator{kN + 1},
                                     std::uint64_t{0});
        const auto stop = std::chrono::steady_clock::now();
        serial_times.push_back(
            std::chrono::duration<double, std::milli>(stop - start).count());
    }
    const double serial_ms = median_ms(serial_times);

    std::printf("N = %llu，串行 accumulate 中位耗时 = %.1f ms\n",
                (unsigned long long)kN, serial_ms);
    std::printf("检查和等于 N(N+1)/2：%s\n", serial_sum == expected ? "是" : "否");

    struct Row {
        int threads;
        double ms;
        std::uint64_t sum;
    };
    std::vector<Row> rows;
    // 先跑一遍 1 线程作为预热，再对每个线程数取 3 次的中位数。
    {
        double warmup = 0;
        (void)parallel_sum(1, warmup);
    }
    for (int threads : {1, 2, 4, 8, 16}) {
        std::vector<double> times;
        std::uint64_t sum = 0;
        for (int repeat = 0; repeat < 3; ++repeat) {
            double ms = 0;
            sum = parallel_sum(threads, ms);
            times.push_back(ms);
        }
        rows.push_back({threads, median_ms(times), sum});
    }

    bool all_ok = serial_sum == expected;
    for (const Row& row : rows) {
        all_ok = all_ok && row.sum == expected;
    }
    std::printf("全部结果一致：%s\n", all_ok ? "是" : "否");
    std::printf("线程数,耗时_ms,加速比\n");
    for (const Row& row : rows) {
        std::printf("%d,%.1f,%.2f\n", row.threads, row.ms, serial_ms / row.ms);
    }
    return all_ok ? 0 : 1;
}
