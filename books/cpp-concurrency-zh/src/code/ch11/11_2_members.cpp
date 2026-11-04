// mode: run
// expect: a = 1, b = 2
// 两个普通的 int 成员是两个内存位置：各写各的，不需要同步。
#include <latch>
#include <iostream>
#include <thread>

struct Pair {
    int a;
    int b;
};

int main() {
    Pair pair{};
    std::latch start{3};

    std::thread ta([&] {
        start.arrive_and_wait();
        pair.a = 1;
    });
    std::thread tb([&] {
        start.arrive_and_wait();
        pair.b = 2;
    });

    start.arrive_and_wait();
    ta.join();
    tb.join();
    std::cout << "a = " << pair.a << ", b = " << pair.b << '\n';
}
