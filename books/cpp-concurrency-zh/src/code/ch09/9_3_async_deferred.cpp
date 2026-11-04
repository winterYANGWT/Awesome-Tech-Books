// mode: run
// expect: deferred 在 get 之前执行: 否
// expect: async 在调用线程执行: 否
// expect: deferred 在调用线程执行: 是
// 同一函数分别用 launch::async 和 launch::deferred 启动。
#include <atomic>
#include <chrono>
#include <cstdio>
#include <future>
#include <thread>
#include <utility>

using namespace std::chrono_literals;

int main() {
    const std::thread::id main_id = std::this_thread::get_id();
    std::atomic<bool> ran_async{false};
    std::atomic<bool> ran_deferred{false};

    // 返回 (是否在调用线程执行, 结果 7)。
    auto job = [&](std::atomic<bool>& ran) {
        ran.store(true, std::memory_order_relaxed);
        return std::make_pair(std::this_thread::get_id() == main_id, 7);
    };

    std::future<std::pair<bool, int>> async_call =
        std::async(std::launch::async, job, std::ref(ran_async));
    std::future<std::pair<bool, int>> deferred_call =
        std::async(std::launch::deferred, job, std::ref(ran_deferred));

    std::this_thread::sleep_for(20ms);
    std::printf("deferred 在 get 之前执行: %s\n",
                ran_deferred.load() ? "是" : "否");

    const std::pair<bool, int> async_result = async_call.get();
    const std::pair<bool, int> deferred_result = deferred_call.get();
    std::printf("async 在调用线程执行: %s\n",
                async_result.first ? "是" : "否");
    std::printf("deferred 在调用线程执行: %s\n",
                deferred_result.first ? "是" : "否");
    std::printf("两个任务都返回 7: %s\n",
                async_result.second == 7 && deferred_result.second == 7
                    ? "是" : "否");
}
