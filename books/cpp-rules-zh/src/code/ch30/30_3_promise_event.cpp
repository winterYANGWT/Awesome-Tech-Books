// 第 30.3 节：用 promise<void>/shared_future<void> 通知多个等待者
// flags: -pthread
#include <atomic>
#include <chrono>
#include <future>
#include <iostream>
#include <thread>
#include <vector>

int main() {
    std::promise<void> event;
    // 多个等待者要各自持有一份 shared_future，它们引用同一个共享状态。
    std::shared_future<void> signal = event.get_future().share();
    std::atomic<int> reacted{0};

    std::vector<std::thread> waiters;
    for (int i = 0; i < 3; ++i) {
        waiters.emplace_back([signal, &reacted] {
            signal.wait();                  // 真正阻塞，不占 CPU 空转
            reacted.fetch_add(1, std::memory_order_relaxed);
        });
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    std::cout << "set_value 之前已响应: " << reacted.load() << '\n';

    event.set_value();                      // 一次性打开闸门
    for (auto& t : waiters) t.join();
    std::cout << "set_value 之后已响应: " << reacted.load() << '\n';
}
