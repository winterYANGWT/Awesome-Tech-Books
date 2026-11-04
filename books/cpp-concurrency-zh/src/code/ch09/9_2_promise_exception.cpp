// mode: run
// expect: 捕获类型: std::runtime_error
// expect: 消息: 数据库连接失败
// 工作线程把异常存进 promise，主线程 get() 时重新抛出。
#include <cstdio>
#include <exception>
#include <future>
#include <stdexcept>
#include <thread>

int main() {
    std::promise<int> promise;
    std::future<int> result = promise.get_future();

    std::thread worker([&promise] {
        try {
            throw std::runtime_error("数据库连接失败");
        } catch (...) {
            // current_exception() 捕获当前正在处理的异常对象。
            promise.set_exception(std::current_exception());
        }
    });

    try {
        const int value = result.get();
        std::printf("不应该走到这里: %d\n", value);
    } catch (const std::runtime_error& e) {
        std::printf("捕获类型: std::runtime_error\n");
        std::printf("消息: %s\n", e.what());
    }
    worker.join();
}
