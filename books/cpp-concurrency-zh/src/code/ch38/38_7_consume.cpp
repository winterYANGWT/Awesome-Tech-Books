// mode: run
// expect: reader: x = 42
#include <atomic>
#include <cstdio>
#include <thread>

struct Data {
    int x;
};

std::atomic<Data*> slot{nullptr};

int main() {
    Data data{0};
    std::thread writer([&] {
        data.x = 42;
        slot.store(&data, std::memory_order_release);
    });
    std::thread reader([&] {
        Data* p = nullptr;
        while ((p = slot.load(std::memory_order_consume)) == nullptr) {
            std::this_thread::yield();
        }
        // kill_dependency 返回同一个指针；它没有可观察的效果。
        Data* q = std::kill_dependency(p);
        std::printf("reader: x = %d\n", q->x);
    });
    writer.join();
    reader.join();
}
