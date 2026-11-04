// mode: race
// expect: data race
#include <cstdio>
#include <memory>
#include <thread>

// 一个 shared_ptr 对象被两个线程同时访问：写的是赋值，读的是 operator bool。
std::shared_ptr<int> g = std::make_shared<int>(0);

void writer() {
    for (int i = 0; i < 100000; ++i) g = std::make_shared<int>(i);
}

void reader() {
    long sum = 0;
    for (int i = 0; i < 100000; ++i) {
        if (g) sum += *g;
    }
    std::printf("reader sum=%ld\n", sum);
}

int main() {
    std::thread a(writer);
    std::thread b(reader);
    a.join();
    b.join();
    std::printf("done: %d\n", g ? *g : -1);
}
