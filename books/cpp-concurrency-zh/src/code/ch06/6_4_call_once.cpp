// mode: run
// 用 call_once 保证 16 个线程里恰好一个执行初始化，其他线程等到它完成。
#include <atomic>
#include <chrono>
#include <iostream>
#include <mutex>
#include <thread>
#include <vector>

using namespace std::chrono_literals;

std::once_flag g_flag;
std::atomic<int> g_calls{0};
std::atomic<int> g_prints{0};
std::atomic<bool> g_ready{false};

void initialize() {
    g_calls.fetch_add(1, std::memory_order_relaxed);
    std::cout << "初始化开始（只应打印一次）\n";
    g_prints.fetch_add(1, std::memory_order_relaxed);
    std::this_thread::sleep_for(10ms);  // 模拟打开连接、读配置
    g_ready.store(true, std::memory_order_release);
}

int main() {
    constexpr int kThreads = 16;
    std::atomic<int> ready_seen{0};
    std::vector<std::thread> threads;
    for (int i = 0; i < kThreads; ++i) {
        threads.emplace_back([&] {
            std::call_once(g_flag, initialize);
            if (g_ready.load(std::memory_order_acquire)) {
                ready_seen.fetch_add(1, std::memory_order_relaxed);
            }
        });
    }
    for (auto& thread : threads) thread.join();

    std::cout << "初始化执行次数=" << g_calls.load() << '\n';
    std::cout << "初始化打印次数=" << g_prints.load() << '\n';
    std::cout << "看到初始化完成的线程=" << ready_seen.load() << " / " << kThreads << '\n';
}
