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
constexpr size_t N = 8u << 20;   // 8M 个 uint64，64 MB
constexpr size_t C = 1024;       // 块-循环的块大小，单位是元素

std::vector<uint64_t> data(N);

uint64_t expected() {
    uint64_t s = 0;
    for (uint64_t v : data) s += v;
    return s;
}

uint64_t sum_block(unsigned p, unsigned t) {
    size_t lo = N * t / p, hi = N * (t + 1) / p;
    uint64_t s = 0;
    for (size_t i = lo; i < hi; ++i) s += data[i];
    return s;
}

uint64_t sum_cyclic(unsigned p, unsigned t) {
    uint64_t s = 0;
    for (size_t i = t; i < N; i += p) s += data[i];
    return s;
}

uint64_t sum_block_cyclic(unsigned p, unsigned t) {
    uint64_t s = 0;
    for (size_t base = size_t(t) * C; base < N; base += size_t(p) * C) {
        size_t hi = std::min(base + C, N);
        for (size_t i = base; i < hi; ++i) s += data[i];
    }
    return s;
}

double run_once(int scheme, unsigned p, uint64_t& got) {
    std::vector<uint64_t> part(p);
    std::vector<std::thread> threads;
    auto t0 = Clock::now();
    for (unsigned t = 0; t < p; ++t) {
        threads.emplace_back([&, t] {
            part[t] = scheme == 0 ? sum_block(p, t)
                      : scheme == 1 ? sum_cyclic(p, t)
                                    : sum_block_cyclic(p, t);
        });
    }
    for (auto& th : threads) th.join();
    auto t1 = Clock::now();
    got = 0;
    for (uint64_t v : part) got += v;
    return std::chrono::duration<double, std::milli>(t1 - t0).count();
}

int main() {
    for (size_t i = 0; i < N; ++i) data[i] = i * 2654435761u;
    uint64_t want = expected();
    std::printf("# N=%zu, block_cyclic chunk=%zu\n", N, C);
    bool ok = true;
    std::printf("scheme,threads,ms\n");
    const char* names[] = {"block", "cyclic", "block_cyclic"};
    for (int scheme = 0; scheme < 3; ++scheme) {
        for (unsigned p : {1u, 2u, 4u, 8u}) {
            std::vector<double> samples;
            uint64_t got = 0;
            for (int r = 0; r < 7; ++r) samples.push_back(run_once(scheme, p, got));
            if (got != want) ok = false;
            std::sort(samples.begin(), samples.end());
            std::printf("%s,%u,%.3f\n", names[scheme], p, samples[samples.size() / 2]);
        }
    }
    std::printf("# 校验: %s\n", ok ? "通过" : "失败");
}
