// 固定一批 CPU 任务，线程数从 1 到 32，看耗时在哪里不再下降。
// mode: run
// output: varies
// tsan: no（计时示例，TSan 的插桩会让耗时失去意义）
#include <atomic>
#include <chrono>
#include <cstdint>
#include <cstdio>
#include <thread>
#include <vector>

namespace {

constexpr int kTasks = 512;
constexpr std::uint32_t kRounds = 200000;

std::uint32_t run_task(std::uint32_t seed) {
    std::uint32_t x = seed | 1u;
    for (std::uint32_t i = 0; i < kRounds; ++i) {
        x ^= x << 13;
        x ^= x >> 17;
        x ^= x << 5;
    }
    return x;
}

// 任务按 i % threads 静态分给线程：没有共享队列，测的是执行体数量本身。
std::uint64_t run_once(int threads) {
    std::vector<std::uint64_t> sums(static_cast<std::size_t>(threads), 0);
    std::vector<std::thread> pool;
    pool.reserve(static_cast<std::size_t>(threads));
    for (int t = 0; t < threads; ++t) {
        pool.emplace_back([t, threads, &sums] {
            std::uint64_t sum = 0;
            for (int i = t; i < kTasks; i += threads) {
                sum += run_task(static_cast<std::uint32_t>(i) * 2654435761u);
            }
            sums[static_cast<std::size_t>(t)] = sum;
        });
    }
    for (auto &th : pool) th.join();
    std::uint64_t total = 0;
    for (auto s : sums) total += s;
    return total;
}

}  // namespace

int main() {
    // 预热一次，避免把第一次的动态链接和页错误算进第一行。
    (void)run_once(4);

    std::printf("threads,ms,checksum\n");
    for (int threads : {1, 2, 4, 8, 16, 24, 32}) {
        const auto begin = std::chrono::steady_clock::now();
        const std::uint64_t sum = run_once(threads);
        const auto end = std::chrono::steady_clock::now();
        const double ms =
            std::chrono::duration<double, std::milli>(end - begin).count();
        std::printf("%d,%.1f,%llu\n", threads, ms,
                    static_cast<unsigned long long>(sum % 1000000007u));
    }
}
