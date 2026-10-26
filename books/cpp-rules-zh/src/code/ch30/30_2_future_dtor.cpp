// 第 30.2 节：future 析构只在“async + launch::async + 最后一个引用”时阻塞
// flags: -pthread
#include <atomic>
#include <chrono>
#include <future>
#include <iostream>
#include <thread>

int slowTask() {
    std::this_thread::sleep_for(std::chrono::milliseconds(200));
    return 7;
}

int main() {
    // 1) async 启动的最后一个 future：析构函数等任务完成，相当于隐式 join。
    std::atomic<bool> slowDone{false};
    {
        auto fut = std::async(std::launch::async, [&slowDone] {
            const int v = slowTask();
            slowDone = true;
            return v;
        });
        std::cout << "async future 离开作用域前，任务完成: " << slowDone.load() << '\n';
    }
    std::cout << "async future 析构返回时，任务完成: " << slowDone.load() << '\n';

    // 2) 共享状态来自 promise：析构只销毁 future，不等任务。
    std::promise<void> gate;
    auto gate_future = gate.get_future();
    std::promise<int> result;
    auto result_future = result.get_future();
    std::atomic<bool> workerDone{false};

    std::thread worker([&] {
        gate_future.wait();              // 等主线程先销毁 result_future
        result.set_value(9);
        workerDone = true;
    });

    {
        auto moved = std::move(result_future);
    }                                     // 最后一个 future 析构，但不会等待
    std::cout << "promise future 析构返回，工作线程仍阻塞: " << !workerDone.load() << '\n';

    gate.set_value();
    worker.join();
    std::cout << "放行后工作线程完成: " << workerDone.load() << '\n';
}
