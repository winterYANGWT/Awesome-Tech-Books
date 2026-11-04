// mode: run
// 函数内局部 static 的初始化从 C++11 起是线程安全的：只构造一次，其他线程等它完成。
#include <atomic>
#include <iostream>
#include <thread>
#include <vector>

struct Settings {
    Settings() { constructions.fetch_add(1, std::memory_order_relaxed); }
    int value = 42;
    static std::atomic<int> constructions;
};
std::atomic<int> Settings::constructions{0};

Settings& settings() {
    static Settings instance;  // 首次调用时构造；并发调用由实现串行化
    return instance;
}

int main() {
    constexpr int kThreads = 16;
    std::vector<const Settings*> seen(kThreads, nullptr);
    std::vector<std::thread> threads;
    for (int i = 0; i < kThreads; ++i) {
        threads.emplace_back([&, i] { seen[i] = &settings(); });
    }
    for (auto& thread : threads) thread.join();

    bool all_same = true;
    for (const Settings* p : seen) all_same = all_same && (p == seen[0]);
    std::cout << "构造次数=" << Settings::constructions.load() << '\n';
    std::cout << "所有地址相同=" << (all_same ? "是" : "否") << '\n';
    std::cout << "value=" << settings().value << '\n';
}
