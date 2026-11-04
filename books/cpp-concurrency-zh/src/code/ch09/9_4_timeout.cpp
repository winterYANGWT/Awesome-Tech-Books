// mode: run
// expect: future.wait_for\(50ms\): timeout
// expect: cv.wait_for\(50ms\): timeout
// expect: 最终结果: 42
// 工作线程 200 ms 后才给结果；主线程用两种 50 ms 超时各等一次。
#include <atomic>
#include <chrono>
#include <condition_variable>
#include <cstdio>
#include <future>
#include <mutex>
#include <thread>

using namespace std::chrono_literals;

int main() {
    std::promise<int> promise;
    std::future<int> result = promise.get_future();
    std::mutex m;
    std::condition_variable cv;
    bool ready = false;
    std::atomic<bool> allow_set{false};

    std::thread worker([&] {
        std::this_thread::sleep_for(200ms);
        // 为了稳定演示，等主线程两次超时都返回后再设值。
        while (!allow_set.load(std::memory_order_acquire)) {
            std::this_thread::sleep_for(1ms);
        }
        promise.set_value(42);
        {
            std::lock_guard<std::mutex> lock(m);
            ready = true;
        }
        cv.notify_one();
    });

    const std::future_status status = result.wait_for(50ms);
    std::printf("future.wait_for(50ms): %s\n",
                status == std::future_status::timeout ? "timeout" : "ready");

    {
        std::unique_lock<std::mutex> lock(m);
        const bool woke = cv.wait_for(lock, 50ms, [&] { return ready; });
        std::printf("cv.wait_for(50ms): %s\n", woke ? "ready" : "timeout");
    }

    allow_set.store(true, std::memory_order_release);
    std::printf("超时返回后任务仍在运行: 是\n");
    std::printf("最终结果: %d\n", result.get());
    worker.join();
}
