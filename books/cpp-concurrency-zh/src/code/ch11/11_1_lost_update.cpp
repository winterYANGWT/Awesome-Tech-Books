// mode: race
// expect: data race
// 两个线程对同一个普通 uint64_t 做 ++：报告会指出读和写落在同一地址。
#include <cstdint>
#include <iostream>
#include <thread>

int main() {
    std::uint64_t counter = 0;
    std::cout << "期望 counter = 2000000\n";

    std::thread a([&counter] {
        for (int i = 0; i < 1'000'000; ++i) {
            ++counter;  // 读、加一、写回，三步之间没有同步
        }
    });
    std::thread b([&counter] {
        for (int i = 0; i < 1'000'000; ++i) {
            ++counter;
        }
    });

    a.join();
    b.join();
    std::cout << "counter = " << counter << '\n';
}
