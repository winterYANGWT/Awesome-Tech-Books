// 第 30.2 节：deferred 任务只在 get/wait 时由调用线程同步执行
// flags: -pthread
#include <chrono>
#include <future>
#include <iostream>

int compute() {
    std::cout << "compute() 开始运行\n";
    return 42;
}

int main() {
    using namespace std::chrono_literals;

    std::cout << "调用 async(launch::deferred)\n";
    auto fut = std::async(std::launch::deferred, compute);
    std::cout << "async 已返回，任务还没运行\n";

    const auto status = fut.wait_for(0ms);
    std::cout << "wait_for(0) 的状态是 deferred: "
              << (status == std::future_status::deferred) << '\n';

    std::cout << "调用 get()\n";
    const int value = fut.get();
    std::cout << "get() 返回 " << value << '\n';

    {
        // deferred 任务如果既不 get 也不 wait，future 析构后永远不会运行。
        auto never = std::async(std::launch::deferred,
                                [] { std::cout << "这一行不会打印\n"; });
    }
    std::cout << "deferred future 已析构，任务没有运行\n";
}
