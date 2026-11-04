// mode: run
// output: varies
// expect: 校验和 200000000
// tsan: no（计时示例，TSan 下太慢）
#include <algorithm>
#include <chrono>
#include <cstdint>
#include <cstdio>
#include <thread>
#include <vector>

using Clock = std::chrono::steady_clock;
constexpr int P = 4;
constexpr long ITERS = 50'000'000;

struct alignas(64) Padded {
    volatile long value;
    char pad[64 - sizeof(long)];
};

template <class F>
double measure(F work) {
    std::vector<double> samples;
    for (int r = 0; r < 3; ++r) {
        auto t0 = Clock::now();
        work();
        auto t1 = Clock::now();
        samples.push_back(std::chrono::duration<double, std::milli>(t1 - t0).count());
    }
    std::sort(samples.begin(), samples.end());
    return samples[samples.size() / 2];
}

int main() {
    // 每个线程只写自己的计数器，逻辑上没有共享；但它们挤在同一条缓存行里。
    std::vector<long> packed(P, 0);
    double bad = measure([&] {
        for (long& v : packed) v = 0;
        std::vector<std::thread> ts;
        for (int t = 0; t < P; ++t) {
            ts.emplace_back([&, t] {
                volatile long* p = packed.data() + t;
                for (long i = 0; i < ITERS; ++i) *p += 1;
            });
        }
        for (auto& th : ts) th.join();
    });

    // 每个计数器独占一条缓存行，多花 64 字节，少让缓存行来回搬。
    std::vector<Padded> spaced(P);
    double good = measure([&] {
        std::vector<std::thread> ts;
        for (int t = 0; t < P; ++t) {
            ts.emplace_back([&, t] {
                volatile long* p = &spaced[t].value;
                for (long i = 0; i < ITERS; ++i) *p += 1;
            });
        }
        for (auto& th : ts) th.join();
    });

    std::printf("每线程 %ld 次自增\n", ITERS);
    std::printf("计数器相邻: %.1f ms\n", bad);
    std::printf("每行一个: %.1f ms\n", good);
    std::printf("对齐后快 %.1f 倍（校验和 %ld）\n", bad / good,
                packed[0] + packed[1] + packed[2] + packed[3]);
}
