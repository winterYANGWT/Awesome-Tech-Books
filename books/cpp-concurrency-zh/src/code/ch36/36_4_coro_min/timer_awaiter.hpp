// 一个只做等待的 awaiter：把 handle 和到期时间交给单线程的定时器队列。
#pragma once

#include <chrono>
#include <coroutine>

struct timer_awaiter {
    std::chrono::milliseconds delay;

    bool await_ready() const noexcept { return delay.count() <= 0; }
    void await_suspend(std::coroutine_handle<> handle);
    void await_resume() const noexcept {}
};

// 睡到最早的一个到期时间，恢复它的协程；队列空时返回 false。
bool run_one_timer();
