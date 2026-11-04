// mode: run
// 对比 launch::async 和 launch::deferred 的启动时机。
#include <atomic>
#include <chrono>
#include <future>
#include <iostream>
#include <latch>

using namespace std::chrono_literals;

const char* status_name(std::future_status s) {
    switch (s) {
    case std::future_status::ready: return "ready";
    case std::future_status::timeout: return "timeout";
    case std::future_status::deferred: return "deferred";
    }
    return "unknown";
}

int main() {
    // async 任务先等在闩上，保证检查状态时它一定还没执行完。
    std::atomic<bool> async_ran{false};
    std::latch release{1};
    auto af = std::async(std::launch::async, [&] {
        release.wait();
        async_ran.store(true, std::memory_order_relaxed);
        return 7;
    });
    std::cout << "async: wait_for(0s)=" << status_name(af.wait_for(0s))
              << ", executed=" << async_ran.load() << '\n';
    release.count_down();
    int ar = af.get();
    std::cout << "async: after get executed=" << async_ran.load()
              << ", result=" << ar << '\n';

    // deferred 任务在 wait_for 时不执行；get 才真正调用它。
    std::atomic<bool> deferred_ran{false};
    auto df = std::async(std::launch::deferred, [&] {
        deferred_ran.store(true, std::memory_order_relaxed);
        return 9;
    });
    std::cout << "deferred: wait_for(0s)=" << status_name(df.wait_for(0s))
              << ", executed=" << deferred_ran.load() << '\n';
    int dr = df.get();
    std::cout << "deferred: after get executed=" << deferred_ran.load()
              << ", result=" << dr << '\n';
}
