// mode: run
// output: varies
// expect: 校验: 通过
// tsan: no（计时示例，TSan 下太慢）
#include <algorithm>
#include <chrono>
#include <cstdint>
#include <cstdio>
#include <thread>
#include <vector>

using Clock = std::chrono::steady_clock;
constexpr uint64_t N = 100'000'000;
constexpr uint64_t WANT = N * (N + 1) / 2;

// 第 i 个线程负责 [i*n/p, (i+1)*n/p)，线程之间不通信。
uint64_t sum_block(uint64_t n, unsigned p, unsigned i) {
    uint64_t lo = n * i / p;
    uint64_t hi = n * (i + 1) / p;
    uint64_t s = 0;
    for (uint64_t v = lo + 1; v <= hi; ++v) s += v;
    return s;
}

double run_once(unsigned p, uint64_t& got) {
    std::vector<uint64_t> part(p);
    std::vector<std::thread> threads;
    auto t0 = Clock::now();
    for (unsigned i = 0; i < p; ++i) {
        threads.emplace_back([&, i] { part[i] = sum_block(N, p, i); });
    }
    for (auto& t : threads) t.join();
    auto t1 = Clock::now();
    got = 0;
    for (uint64_t v : part) got += v;
    return std::chrono::duration<double, std::milli>(t1 - t0).count();
}

int main() {
    std::printf("N=%llu，按连续块划分\n", (unsigned long long)N);
    uint64_t last = 0;
    for (unsigned p : {1u, 2u, 4u, 8u}) {
        std::vector<double> samples;
        for (int r = 0; r < 5; ++r) samples.push_back(run_once(p, last));
        std::sort(samples.begin(), samples.end());
        std::printf("线程 %u: %.3f ms  部分和之和=%llu\n", p, samples[samples.size() / 2],
                    (unsigned long long)last);
    }
    std::printf("校验: %s（闭式解 %llu）\n", last == WANT ? "通过" : "失败",
                (unsigned long long)WANT);
}
