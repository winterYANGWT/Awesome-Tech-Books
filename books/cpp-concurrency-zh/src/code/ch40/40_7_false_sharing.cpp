// mode: run
// tsan: no（计时示例）
// output: varies
// expect: sizeof_packed=16 sizeof_padded=128 line=64
// expect: packed_ms=
// expect: padded_ms=
// expect: ratio=

#include <algorithm>
#include <chrono>
#include <cstdint>
#include <cstdio>
#include <new>
#include <thread>
#include <vector>

// 两个计数器放在同一缓存行；每个线程只写自己的成员，逻辑上没有竞争。
struct Packed {
    volatile std::uint64_t a;
    volatile std::uint64_t b;
};

// 各占一行。64 是 x86-64 的常见值；跨平台应查 hardware_destructive_interference_size。
struct alignas(64) Padded {
    alignas(64) volatile std::uint64_t a;
    alignas(64) volatile std::uint64_t b;
};

template <typename Counters>
double bench(Counters& counters, std::uint64_t iterations, int reps, std::uint64_t& sum) {
    std::vector<double> samples;
    for (int rep = 0; rep < reps; ++rep) {
        const auto start = std::chrono::steady_clock::now();
        std::thread first([&] {
            for (std::uint64_t i = 0; i < iterations; ++i) counters.a = counters.a + 1;
        });
        std::thread second([&] {
            for (std::uint64_t i = 0; i < iterations; ++i) counters.b = counters.b + 1;
        });
        first.join();
        second.join();
        const auto stop = std::chrono::steady_clock::now();
        samples.push_back(std::chrono::duration<double, std::milli>(stop - start).count());
    }
    std::sort(samples.begin(), samples.end());
    sum = counters.a + counters.b;
    return samples[samples.size() / 2];
}

int main() {
    constexpr std::uint64_t kIterations = 20'000'000;
    Packed packed{};
    Padded padded{};
    std::uint64_t packed_sum = 0;
    std::uint64_t padded_sum = 0;

    const double packed_ms = bench(packed, kIterations, 5, packed_sum);
    const double padded_ms = bench(padded, kIterations, 5, padded_sum);

    std::printf("sizeof_packed=%zu sizeof_padded=%zu line=%zu\n", sizeof(Packed), sizeof(Padded),
                std::hardware_destructive_interference_size);
    std::printf("packed_ms=%.3f padded_ms=%.3f ratio=%.2f\n", packed_ms, padded_ms,
                packed_ms / padded_ms);
    std::printf("sums=%llu,%llu\n", static_cast<unsigned long long>(packed_sum),
                static_cast<unsigned long long>(padded_sum));
}
