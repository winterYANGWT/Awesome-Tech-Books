// mode: race
// expect: data race
// 两个线程对同一个普通 uint64_t 做 ++，TSan 会在这里停下来。
#include <cstdint>
#include <iostream>
#include <thread>

int main() {
    std::uint64_t counter = 0;
    std::cout << "两个线程各对 counter 加 1000000 次\n";

    std::thread a([&] {
        for (int i = 0; i < 1'000'000; ++i) {
            ++counter;  // 读、加一、写回，三步之间没有同步
        }
    });
    std::thread b([&] {
        for (int i = 0; i < 1'000'000; ++i) {
            ++counter;
        }
    });

    a.join();
    b.join();
    std::cout << "counter = " << counter << '\n';
}
