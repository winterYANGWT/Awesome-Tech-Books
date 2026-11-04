// mode: run
// expect: 10 轮全部通过
// 4 个线程做 10 轮“各算一段、一起汇总”：完成函数在每轮到齐后由一个线程执行一次。
#include <barrier>

#include <atomic>
#include <cstdint>
#include <cstdio>
#include <thread>
#include <vector>

constexpr int kThreads = 4;
constexpr int kRounds = 10;
constexpr int kPerThread = 1000;

uint64_t partials[kThreads];
uint64_t round_total = 0;
uint64_t expected_total[kRounds];
std::atomic<int> failures{0};

std::barrier round_barrier{kThreads, [] {
    // 所有线程都写完了 partials，才轮到完成函数执行
    uint64_t sum = 0;
    for (uint64_t v : partials) sum += v;
    round_total = sum;
}};

int main() {
    for (int r = 0; r < kRounds; ++r) {
        uint64_t lo = static_cast<uint64_t>(r) * kThreads * kPerThread + 1;
        expected_total[r] = (lo + lo + kThreads * kPerThread - 1) * kThreads * kPerThread / 2;
    }

    std::vector<std::thread> threads;
    for (int i = 0; i < kThreads; ++i) {
        threads.emplace_back([i] {
            for (int r = 0; r < kRounds; ++r) {
                uint64_t lo = static_cast<uint64_t>(r) * kThreads * kPerThread + 1 + i * kPerThread;
                uint64_t sum = 0;
                for (int k = 0; k < kPerThread; ++k) sum += lo + k;
                partials[i] = sum;
                round_barrier.arrive_and_wait();  // 等完成函数把 round_total 算好
                if (i == 0) {
                    bool ok = round_total == expected_total[r];
                    if (!ok) failures.fetch_add(1);
                    std::printf("第 %2d 轮: 总和 = %llu，%s\n", r + 1,
                                static_cast<unsigned long long>(round_total),
                                ok ? "校验通过" : "校验失败");
                }
            }
        });
    }
    for (auto& t : threads) t.join();
    if (failures.load() == 0) std::printf("%d 轮全部通过\n", kRounds);
}
