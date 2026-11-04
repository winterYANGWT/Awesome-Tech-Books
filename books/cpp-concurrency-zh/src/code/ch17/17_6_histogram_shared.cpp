// mode: race
// expect: data race
#include <atomic>
#include <cstdint>
#include <cstdio>
#include <thread>
#include <vector>

constexpr int kBins = 8;
constexpr int kThreads = 4;
constexpr int N = 2'000'000;

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

    // 所有线程直接更新同一个计数数组：没有锁，也没有原子操作。
    std::vector<uint64_t> bin_counts(kBins, 0);
    std::vector<std::thread> threads;
    for (int t = 0; t < kThreads; ++t) {
        threads.emplace_back([&, t] {
            int lo = N * t / kThreads, hi = N * (t + 1) / kThreads;
            for (int i = lo; i < hi; ++i) ++bin_counts[bin_of(data[i])];
        });
    }
    for (auto& th : threads) th.join();

    uint64_t sum = 0;
    for (uint64_t v : bin_counts) sum += v;
    std::printf("总数: %llu（期望 %d）\n", (unsigned long long)sum, N);
}
