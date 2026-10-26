// 第 30.4 节：volatile 不提供原子性，两次自增可能只留下一次
// flags: -pthread
#include <future>
#include <iostream>
#include <thread>

volatile int counter = 0;

int main() {
    std::promise<void> a_read, b_read, a_wrote;
    auto a_read_done = a_read.get_future();
    auto b_read_done = b_read.get_future();
    auto a_write_done = a_wrote.get_future();

    // 两个线程各做一次“读、加一、写回”，用手势把交错顺序固定下来，
    // 保证两次读都发生在写之前；这样没有并发访问，只有丢失的更新。
    std::thread a([&] {
        const int value = counter;
        std::cout << "A 读到 " << value << '\n';
        a_read.set_value();
        b_read_done.wait();
        counter = value + 1;
        std::cout << "A 写入 " << counter << '\n';
        a_wrote.set_value();
    });

    std::thread b([&] {
        a_read_done.wait();
        const int value = counter;
        std::cout << "B 读到 " << value << '\n';
        b_read.set_value();
        a_write_done.wait();
        counter = value + 1;
        std::cout << "B 写入 " << counter << '\n';
    });

    a.join();
    b.join();
    std::cout << "两次自增后 counter = " << counter << '\n';
}
