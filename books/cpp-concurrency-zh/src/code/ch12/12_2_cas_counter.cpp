// mode: run
// expect: counter = 400000
#include <atomic>
#include <iostream>
#include <thread>

// 用 CAS 循环自己实现 fetch_add；失败时 expected 已被写成当前值
void add_one(std::atomic<int>& value) {
    int expected = value.load();
    while (!value.compare_exchange_weak(expected, expected + 1)) {
        // 循环体留空：失败路径更新了 expected，下一轮直接用它重试
    }
}

int main() {
    constexpr int kThreads = 4;
    constexpr int kIncrements = 100'000;
    std::atomic<int> counter{0};

    auto add = [&] {
        for (int i = 0; i < kIncrements; ++i) {
            add_one(counter);
        }
    };

    std::thread a(add);
    std::thread b(add);
    std::thread c(add);
    std::thread d(add);
    a.join();
    b.join();
    c.join();
    d.join();

    std::cout << kThreads << " 个线程，counter = " << counter.load() << '\n';
}
