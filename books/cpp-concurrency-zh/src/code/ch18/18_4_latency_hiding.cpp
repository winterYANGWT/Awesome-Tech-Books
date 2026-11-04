// mode: run
// output: varies
// tsan: no（计时示例，大量 sleep）
// timeout: 60
#include <atomic>
#include <chrono>
#include <cstdio>
#include <thread>
#include <vector>

namespace {

constexpr int connections = 32;  // 同时在等 I/O 的任务数
constexpr int rounds = 16;       // 每个连接要等的次数
constexpr int requests = connections * rounds;

double run(unsigned threads) {
    std::atomic<int> next{0};
    const auto start = std::chrono::steady_clock::now();
    std::vector<std::thread> pool;
    pool.reserve(threads);
    for (unsigned t = 0; t < threads; ++t) {
        pool.emplace_back([&] {
            for (;;) {
                const int id = next.fetch_add(1, std::memory_order_relaxed);
                if (id >= connections) break;
                for (int r = 0; r < rounds; ++r) {
                    std::this_thread::sleep_for(std::chrono::milliseconds(2));
                }
            }
        });
    }
    for (auto& th : pool) th.join();
    const auto stop = std::chrono::steady_clock::now();
    return std::chrono::duration<double>(stop - start).count();
}

}  // namespace

int main() {
    std::puts("threads,tasks_per_sec");
    for (unsigned p : {1u, 2u, 4u, 8u, 16u, 32u, 48u, 64u}) {
        const double seconds = run(p);
        std::printf("%u,%.1f\n", p, requests / seconds);
    }
}
