// 第 30.2 节：任务式并发能取回返回值，也能把异常带回调用方
// flags: -pthread
#include <future>
#include <iostream>
#include <stdexcept>
#include <thread>
#include <vector>

int price(int id) { return id * 10; }

int fail() { throw std::runtime_error("order 3 is invalid"); }

int main() {
    const auto caller = std::this_thread::get_id();

    // launch::async 要求在另一个线程上执行，这里直接验证这一点。
    auto where = std::async(std::launch::async,
                            [caller] { return std::this_thread::get_id() != caller; });
    std::cout << "task 与调用线程不同: " << std::boolalpha << where.get() << '\n';

    auto good = std::async(std::launch::async, price, 7);
    std::cout << "price(7): " << good.get() << '\n';

    // 线程函数抛出的异常保存在共享状态里，由 get() 在调用方重新抛出。
    auto bad = std::async(std::launch::async, fail);
    try {
        bad.get();
    } catch (const std::exception& e) {
        std::cout << "get 重新抛出: " << e.what() << '\n';
    }

    // 一次提交多个独立任务，再把结果收集起来；线程怎么复用由实现决定。
    std::vector<std::future<int>> tasks;
    for (int i = 0; i < 8; ++i)
        tasks.push_back(std::async(std::launch::async, price, i));

    int total = 0;
    for (auto& f : tasks) total += f.get();
    std::cout << "8 个任务求和: " << total << '\n';
}
