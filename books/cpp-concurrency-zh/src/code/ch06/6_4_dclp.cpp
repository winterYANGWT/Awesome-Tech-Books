// mode: race
// expect: data race
// 双重检查锁：锁外读 g_config，锁内写 g_config，两者之间没有 happens-before。
#include <cstdint>
#include <iostream>
#include <mutex>
#include <thread>
#include <vector>

struct Config {
    std::uint64_t a;
    std::uint64_t b;
    Config() : a(1), b(2) {}
};

Config* g_config = nullptr;
std::mutex g_mutex;

Config* get_config() {
    if (g_config == nullptr) {                 // 锁外读：数据竞争
        std::lock_guard<std::mutex> lock(g_mutex);
        if (g_config == nullptr) {
            g_config = new Config;             // 锁内写指针，同时构造对象
        }
    }
    return g_config;
}

int main() {
    std::vector<std::thread> threads;
    for (int t = 0; t < 8; ++t) {
        threads.emplace_back([] {
            std::uint64_t sum = 0;
            for (int i = 0; i < 1000; ++i) {
                Config* p = get_config();
                sum += p->a + p->b;
            }
            std::cout << sum << '\n';
        });
    }
    for (auto& thread : threads) thread.join();
    delete g_config;
}
