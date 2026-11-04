// mode: run
// expect: counter = 2000000
#include <atomic>
#include <iostream>
#include <thread>

int main() {
    constexpr int kThreads = 2;
    constexpr int kIncrements = 1'000'000;
    std::atomic<int> counter{0};

    auto add = [&] {
        for (int i = 0; i < kIncrements; ++i) {
            // 读-改-写是一条原子指令，别的线程看不到“做了一半”的状态
            counter.fetch_add(1);
        }
    };

    std::thread a(add);
    std::thread b(add);
    a.join();
    b.join();

    std::cout << kThreads << " 个线程，counter = " << counter.load() << '\n';
}
