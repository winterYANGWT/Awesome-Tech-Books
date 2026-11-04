// mode: run
// output: varies
// expect: 读到 x != y 的有 [1-9]\d* 次
// 两个字段各自是原子的，但“一起更新”不是原子的：
// 写线程先写 x 再写 y，读线程先读 y 再读 x，就会看到中间状态。
#include <atomic>
#include <iostream>
#include <thread>

int main() {
    constexpr int kUpdates = 5'000'000;
    std::atomic<int> x{0};
    std::atomic<int> y{0};
    long long mismatches = 0;

    // ANCHOR: pair
    std::thread writer([&] {
        for (int i = 1; i <= kUpdates; ++i) {
            x.store(i, std::memory_order_relaxed);
            y.store(i, std::memory_order_relaxed);
        }
    });
    std::thread reader([&] {
        for (int i = 0; i < kUpdates; ++i) {
            const int seen_y = y.load(std::memory_order_relaxed);
            const int seen_x = x.load(std::memory_order_relaxed);
            if (seen_x != seen_y) {
                ++mismatches;  // 读到了“只更新了一个字段”的中间状态
            }
        }
    });

    // ANCHOR_END: pair
    writer.join();
    reader.join();
    std::cout << "写线程更新 " << kUpdates << " 次，读线程检查 " << kUpdates << " 次\n";
    std::cout << "读到 x != y 的有 " << mismatches << " 次\n";
}
