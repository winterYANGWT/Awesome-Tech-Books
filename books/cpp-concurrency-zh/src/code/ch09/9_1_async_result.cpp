// mode: run
// expect: get 之前任务已开始: 是
// expect: 任务在独立线程: 是
// expect: 结果: 42
// 用 std::async 在后台算一个整数，主线程先做别的事，再 get 取结果。
#include <atomic>
#include <chrono>
#include <cstdio>
#include <future>
#include <thread>

using namespace std::chrono_literals;

int main() {
    const std::thread::id main_id = std::this_thread::get_id();
    std::atomic<bool> started{false};
    std::atomic<bool> on_worker{false};

    std::future<int> result = std::async(std::launch::async, [&] {
        on_worker.store(std::this_thread::get_id() != main_id,
                        std::memory_order_relaxed);
        started.store(true, std::memory_order_release);
        std::this_thread::sleep_for(30ms);
        return 42;
    });

    // 等任务真正跑起来，再证明 get 之前它已经在工作。
    while (!started.load(std::memory_order_acquire)) {
        std::this_thread::sleep_for(1ms);
    }
    std::printf("get 之前任务已开始: 是\n");

    // 主线程先做一段与结果无关的工作。
    unsigned long long work = 0;
    for (int i = 0; i < 100000; ++i) {
        work += static_cast<unsigned long long>(i);
    }
    std::printf("主线程在 get 之前做完了别的工作: %s\n",
                work == 4999950000ULL ? "是" : "否");

    const int value = result.get();
    std::printf("任务在独立线程: %s\n", on_worker.load() ? "是" : "否");
    std::printf("结果: %d\n", value);
}
