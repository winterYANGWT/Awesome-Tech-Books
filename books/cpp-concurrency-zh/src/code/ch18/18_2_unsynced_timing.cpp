// mode: race
// expect: data race
#include <atomic>
#include <cstdint>
#include <iostream>
#include <thread>

int main() {
    constexpr int n = 20'000'000;
    std::atomic<bool> started{false};
    int progress = 0;
    long long result = 0;

    std::thread worker([&] {
        started.store(true, std::memory_order_release);
        for (int i = 1; i <= n; ++i) {
            result += i & 1;
            progress = i;  // 主线程在 join 之前读它
        }
    });

    while (!started.load(std::memory_order_acquire)) {
        std::this_thread::yield();
    }
    while (progress < n / 2) {
        std::this_thread::yield();
    }
    std::cout << "midway: progress=" << progress << " result=" << result
              << "\n";

    worker.join();
    std::cout << "final: progress=" << progress << " result=" << result
              << "\n";
}
