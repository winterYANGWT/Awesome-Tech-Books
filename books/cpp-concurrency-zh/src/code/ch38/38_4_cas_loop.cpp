// mode: run
// expect: max = 4000
#include <atomic>
#include <cstdio>
#include <thread>
#include <vector>

bool atomic_halve(std::atomic<int>& v, int& steps) {
    int old = v.load(std::memory_order_relaxed);
    while (old > 1) {
        int desired = old / 2;
        if (v.compare_exchange_weak(old, desired, std::memory_order_relaxed)) {
            ++steps;
            old = desired;
        }
        // 失败时 expected（old）已被写回当前值，下一轮直接用。
    }
    return old == 1;
}

void atomic_raise(std::atomic<int>& best, int value) {
    int old = best.load(std::memory_order_relaxed);
    while (old < value &&
           !best.compare_exchange_weak(old, value, std::memory_order_relaxed)) {
        // 失败时 old 被更新成当前值，继续比较。
    }
}

int main() {
    std::atomic<int> n{100};
    int steps = 0;
    (void)atomic_halve(n, steps);
    std::printf("100 -> 1: steps=%d, n=%d\n", steps, n.load());

    // 失败时 expected 会被写回当前值：这里故意拿 41 去换 42。
    std::atomic<int> x{42};
    int expected = 41;
    bool ok = x.compare_exchange_weak(expected, 0);
    std::printf("failed CAS: ok=%d expected=%d x=%d\n", ok, expected, x.load());

    std::atomic<int> best{0};
    std::vector<std::thread> threads;
    for (int t = 0; t < 4; ++t) {
        threads.emplace_back([&best, t] {
            for (int i = 1; i <= 1000; ++i) {
                atomic_raise(best, t * 1000 + i);
            }
        });
    }
    for (auto& th : threads) th.join();
    // C++26 可以写成 best.fetch_max(value)；本书环境未实现，未编译验证。
    std::printf("max = %d\n", best.load());
}
