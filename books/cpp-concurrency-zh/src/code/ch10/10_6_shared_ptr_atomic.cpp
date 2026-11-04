// mode: run
// output: varies
// expect: 非法值: 0\nis_lock_free: false
#include <atomic>
#include <cstdio>
#include <memory>
#include <thread>

// C++20 的 atomic<shared_ptr<T>>：load/store 是原子的，但本机不是无锁。
std::atomic<std::shared_ptr<int>> g{std::make_shared<int>(1)};

int main() {
    std::atomic<int> ones{0};
    std::atomic<int> twos{0};
    std::atomic<int> bad{0};

    std::thread writer([] {
        for (int i = 0; i < 200000; ++i) {
            g.store(std::make_shared<int>(i % 2 + 1));
        }
    });
    std::thread reader([&] {
        for (int i = 0; i < 200000; ++i) {
            std::shared_ptr<int> p = g.load();
            if (!p) {
                ++bad;
            } else if (*p == 1) {
                ++ones;
            } else if (*p == 2) {
                ++twos;
            } else {
                ++bad;
            }
        }
    });
    writer.join();
    reader.join();

    std::printf("读到 1 的次数: %d\n", ones.load());
    std::printf("读到 2 的次数: %d\n", twos.load());
    std::printf("非法值: %d\n", bad.load());
    std::printf("is_lock_free: %s\n", g.is_lock_free() ? "true" : "false");
}
