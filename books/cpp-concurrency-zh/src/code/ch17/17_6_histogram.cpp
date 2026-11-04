// mode: run
// expect: 校验: 通过
#include <array>
#include <cstdint>
#include <cstdio>
#include <thread>
#include <vector>

constexpr int kBins = 8;
constexpr int kThreads = 4;
constexpr int N = 4'000'000;

int bin_of(double v) {
    int b = int(v * kBins);
    return b >= kBins ? kBins - 1 : b;
}

int main() {
    std::vector<double> data(N);
    uint64_t s = 88172645463325252ull;
    for (double& v : data) {
        s ^= s << 13;
        s ^= s >> 7;
        s ^= s << 17;
        v = double(s >> 11) * 0x1.0p-53;
    }

    // 每个线程一份本地计数：写的是自己的栈，不需要同步。
    std::array<uint64_t, kBins> local[kThreads]{};
    std::vector<std::thread> threads;
    for (int t = 0; t < kThreads; ++t) {
        threads.emplace_back([&, t] {
            int lo = N * t / kThreads, hi = N * (t + 1) / kThreads;
            for (int i = lo; i < hi; ++i) ++local[t][bin_of(data[i])];
        });
    }
    for (auto& th : threads) th.join();

    // 合并 4 份本地数组，这一步是串行的。
    std::array<uint64_t, kBins> total{};
    for (int t = 0; t < kThreads; ++t)
        for (int b = 0; b < kBins; ++b) total[b] += local[t][b];

    // 串行版做对照，确认桶号计算和划分都没有漏项。
    std::array<uint64_t, kBins> serial{};
    for (double v : data) ++serial[bin_of(v)];

    uint64_t sum = 0;
    for (uint64_t v : total) sum += v;
    std::printf("桶:     ");
    for (int b = 0; b < kBins; ++b) std::printf("%8d", b);
    std::printf("\n并行计数:");
    for (uint64_t v : total) std::printf("%8llu", (unsigned long long)v);
    std::printf("\n串行计数:");
    for (uint64_t v : serial) std::printf("%8llu", (unsigned long long)v);
    std::printf("\n总数: %llu\n", (unsigned long long)sum);
    bool ok = sum == N && total == serial;
    std::printf("校验: %s\n", ok ? "通过" : "失败");
}
