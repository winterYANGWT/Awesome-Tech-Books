// mode: run
// expect: consumer: value=42 spins=0
#include <atomic>
#include <chrono>
#include <cstdio>
#include <thread>

std::atomic<int> value{0};
std::atomic<int> spins{0};

int main() {
    std::thread consumer([] {
        int v = value.load(std::memory_order_acquire);
        while (v == 0) {
            value.wait(0, std::memory_order_acquire);
            int next = value.load(std::memory_order_acquire);
            if (next == 0) {
                spins.fetch_add(1, std::memory_order_relaxed);
            }
            v = next;
        }
        std::printf("consumer: value=%d spins=%d\n", v,
                    spins.load(std::memory_order_relaxed));
    });

    // 给消费者时间进入 wait；这不是同步，只是让等待真的发生。
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    value.store(42, std::memory_order_release);
    value.notify_one();
    consumer.join();
}
