// mode: run
// output: varies
// tsan: no（计时示例，TSan 会放大调度开销）
// timeout: 60
// expect: ^线程数,实测,Amdahl 模型,上限 1/r
#include <algorithm>
#include <chrono>
#include <cstdint>
#include <iomanip>
#include <iostream>
#include <thread>
#include <vector>

using Clock = std::chrono::steady_clock;

constexpr std::uint64_t kParallel = 40'000'000;
constexpr std::uint64_t kSerial = kParallel / 9;  // 串行段约占 10%

// 有依赖的 xorshift，编译器不能把它折叠成闭式公式。
std::uint64_t work(std::uint64_t begin, std::uint64_t end) {
    std::uint64_t x = begin | 1;
    std::uint64_t s = 0;
    for (std::uint64_t i = begin; i < end; ++i) {
        x ^= x << 13;
        x ^= x >> 7;
        x ^= x << 17;
        s += x + i;
    }
    return s;
}

double run_once(unsigned threads) {
    volatile std::uint64_t sink = 0;
    const auto start = Clock::now();

    // 串行段：只有调用线程做。
    sink = sink + work(0, kSerial);

    // 并行段：每个线程算一段，结果写进自己的槽。
    std::vector<std::uint64_t> partial(threads);
    std::vector<std::thread> pool;
    const std::uint64_t chunk = kParallel / threads;
    for (unsigned i = 0; i < threads; ++i) {
        const std::uint64_t b = i * chunk;
        const std::uint64_t e = (i + 1 == threads) ? kParallel : b + chunk;
        pool.emplace_back([&partial, i, b, e] { partial[i] = work(b, e); });
    }
    for (auto& t : pool) t.join();

    std::uint64_t total = 0;
    for (auto v : partial) total += v;
    sink = sink + total;

    const auto stop = Clock::now();
    (void)sink;
    return std::chrono::duration<double, std::milli>(stop - start).count();
}

double best_of_three(unsigned threads) {
    double best = 1e100;
    for (int rep = 0; rep < 3; ++rep) best = std::min(best, run_once(threads));
    return best;
}

int main() {
    // 预热：先跑两次，避开第一次建线程和页表的开销。
    run_once(1);
    run_once(1);

    const std::vector<unsigned> counts{1, 2, 4, 8, 16};
    std::vector<double> times;
    for (unsigned n : counts) times.push_back(best_of_three(n));

    const double baseline = times.front();
    std::cout << "线程数,实测,Amdahl 模型,上限 1/r\n";
    std::cout << std::fixed << std::setprecision(2);
    for (std::size_t i = 0; i < counts.size(); ++i) {
        const double measured = baseline / times[i];
        const double model = 1.0 / (0.1 + 0.9 / counts[i]);
        std::cout << counts[i] << ',' << measured << ',' << model << ',' << 10.0
                  << '\n';
    }
}
