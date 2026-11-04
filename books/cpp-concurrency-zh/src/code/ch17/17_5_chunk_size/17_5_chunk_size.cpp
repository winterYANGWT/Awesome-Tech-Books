// mode: run
// output: varies
// expect: chunk,ms
// tsan: no（计时示例，TSan 下太慢）
// stdout 是总耗时；单块 p99 写进 p99.out，给第二张图读。
#include <algorithm>
#include <atomic>
#include <chrono>
#include <cstdint>
#include <cstdio>
#include <thread>
#include <vector>

using Clock = std::chrono::steady_clock;
constexpr int kWorkers = 4;
constexpr int kRows = 1024;

struct Chunk { int row, lo, hi; };

int main() {
    std::printf("chunk,ms\n");
    std::FILE* p99_out = std::fopen("p99.out", "w");
    if (!p99_out) return 1;
    std::fprintf(p99_out, "chunk,p99_ns\n");
    uint64_t checksum = 0;
    for (int chunk : {1, 4, 16, 64, 256, 1024, 4096}) {
        std::vector<Chunk> chunks;
        for (int r = 0; r < kRows; ++r)
            for (int j = 0; j <= r; j += chunk)
                chunks.push_back({r, j, std::min(j + chunk, r + 1)});

        std::vector<std::pair<double, double>> samples;
        for (int rep = 0; rep < 3; ++rep) {
            std::vector<uint32_t> dur(chunks.size());
            std::atomic<size_t> next{0};
            std::vector<uint64_t> part(kWorkers);
            auto t0 = Clock::now();
            std::vector<std::thread> workers;
            for (int t = 0; t < kWorkers; ++t) {
                workers.emplace_back([&, t] {
                    uint64_t acc = 0;
                    for (;;) {
                        size_t i = next.fetch_add(1, std::memory_order_relaxed);
                        if (i >= chunks.size()) break;
                        auto a = Clock::now();
                        Chunk c = chunks[i];
                        for (int j = c.lo; j < c.hi; ++j)
                            acc += uint64_t(c.row + 1) * uint64_t(j + 3);
                        auto b = Clock::now();
                        dur[i] = uint32_t(
                            std::chrono::duration_cast<std::chrono::nanoseconds>(b - a).count());
                    }
                    part[t] = acc;
                });
            }
            for (auto& w : workers) w.join();
            auto t1 = Clock::now();
            double ms = std::chrono::duration<double, std::milli>(t1 - t0).count();
            std::sort(dur.begin(), dur.end());
            double p99_ns = double(dur[size_t(0.99 * double(dur.size() - 1))]);
            samples.emplace_back(ms, p99_ns);
            uint64_t sum = 0;
            for (uint64_t v : part) sum += v;
            checksum = sum;
        }
        std::sort(samples.begin(), samples.end());
        std::printf("%d,%.3f\n", chunk, samples[1].first);
        std::fprintf(p99_out, "%d,%.0f\n", chunk, samples[1].second);
    }
    std::fclose(p99_out);
    // 结果参与退出码，防止整个循环被优化掉。
    if (checksum == 0) return 1;
}
