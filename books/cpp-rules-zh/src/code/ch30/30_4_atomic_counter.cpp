// 第 30.4 节：共享计数器用 atomic；发布数据用 release/acquire 建立先后关系
// flags: -pthread
#include <atomic>
#include <iostream>
#include <thread>
#include <vector>

int main() {
    constexpr int kThreads = 4;
    constexpr int kPerThread = 25000;
    std::atomic<int> count = 0;   // C++17 起这种写法合法

    std::vector<std::thread> workers;
    for (int t = 0; t < kThreads; ++t) {
        workers.emplace_back([&count] {
            for (int i = 0; i < kPerThread; ++i)
                ++count;                     // 原子读-改-写，不会丢更新
        });
    }
    for (auto& t : workers) t.join();
    std::cout << "原子计数: " << count.load() << "（期望 " << kThreads * kPerThread << "）\n";

    // payload 是普通 int，靠 ready 的 release/acquire 保证消费者能看到写入。
    std::atomic<bool> ready{false};
    int payload = 0;
    std::thread producer([&] {
        payload = 12345;
        ready.store(true, std::memory_order_release);
    });
    std::thread consumer([&] {
        while (!ready.load(std::memory_order_acquire)) {
            std::this_thread::yield();
        }
        std::cout << "消费者读到 payload: " << payload << '\n';
    });
    producer.join();
    consumer.join();
}
