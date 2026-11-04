// mode: run
// expect: promise = 42
// 三种建立先行发生关系的设施各做一次发布，数据都在发布之前写好。
#include <future>
#include <iostream>
#include <latch>
#include <thread>

int main() {
    // 1. promise / future：set_value 与 get 同步。
    int data1 = 0;
    std::promise<int> promise;
    std::future<int> future = promise.get_future();
    std::thread writer1([&] {
        data1 = 42;
        promise.set_value(data1);
    });
    const int got1 = future.get();
    writer1.join();

    // 2. latch：count_down 与 wait 返回同步。
    int data2 = 0;
    std::latch published{1};
    std::thread writer2([&] {
        data2 = 43;
        published.count_down();
    });
    published.wait();
    const int got2 = data2;
    writer2.join();

    // 3. 线程构造：构造完成与新线程开始执行同步。
    int data3 = 44;
    int got3 = 0;
    std::thread writer3([&] { got3 = data3; });
    writer3.join();

    std::cout << "promise = " << got1 << '\n'
              << "latch = " << got2 << '\n'
              << "thread start = " << got3 << '\n';
}
