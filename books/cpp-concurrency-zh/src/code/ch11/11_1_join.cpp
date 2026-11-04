// mode: run
// expect: 2000000
// 每个线程只写自己的结果槽；两个槽是不同的内存位置，join 后再合并。
#include <array>
#include <cstdint>
#include <iostream>
#include <thread>

int main() {
    constexpr int kThreads = 2;
    constexpr int kPerThread = 1'000'000;
    std::array<std::uint64_t, kThreads> results{};

    auto work = [&results](int slot) {
        for (int i = 0; i < kPerThread; ++i) {
            ++results[slot];
        }
    };

    std::thread a(work, 0);
    std::thread b(work, 1);
    a.join();  // 返回时，线程函数的完成与这里同步
    b.join();

    const std::uint64_t total = results[0] + results[1];
    std::cout << results[0] << " + " << results[1] << " = " << total << '\n';
}
