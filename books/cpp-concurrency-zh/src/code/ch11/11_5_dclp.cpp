// mode: race
// expect: data race
// 双重检查锁：第一次用普通指针读，锁只在第二次检查之后才起作用。
#include <iostream>
#include <mutex>
#include <thread>

struct Config {
    int value;
};

Config *config = nullptr;
std::mutex init_mutex;

Config *get_config() {
    if (config == nullptr) {  // 锁外的第一次检查：普通读
        std::lock_guard<std::mutex> lock(init_mutex);
        if (config == nullptr) {
            config = new Config{42};  // 锁内写，但和上面的读没有同步
        }
    }
    return config;
}

int main() {
    std::cout << "两个线程同时取 Config\n";
    std::thread a([] { (void)get_config(); });
    std::thread b([] { (void)get_config(); });
    a.join();
    b.join();
    std::cout << "value = " << get_config()->value << '\n';
}
