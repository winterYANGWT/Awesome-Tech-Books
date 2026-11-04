// mode: run
// expect: 闭式解与原子累加一致
#include <atomic>
#include <cstdint>
#include <iostream>
#include <thread>
#include <vector>

int main() {
    constexpr std::uint64_t kN = 10'000'000;
    constexpr int kThreads = 4;
    std::atomic<std::uint64_t> sum{0};
    std::vector<std::thread> threads;

    for (int t = 0; t < kThreads; ++t) {
        threads.emplace_back([&, t] {
            const std::uint64_t begin = kN * t / kThreads;
            const std::uint64_t end = kN * (t + 1) / kThreads;
            for (std::uint64_t i = begin; i < end; ++i) {
                // 1..N 求和：每个线程把自己那一段逐个加到共享累加器
                sum.fetch_add(i + 1);
            }
        });
    }
    for (std::thread& th : threads) {
        th.join();
    }

    const std::uint64_t expected = kN * (kN + 1) / 2;
    const std::uint64_t got = sum.load();
    std::cout << "sum(1.." << kN << ") = " << got << '\n';
    std::cout << "闭式解 = " << expected << '\n';
    std::cout << (got == expected ? "闭式解与原子累加一致" : "结果不一致") << '\n';
}
