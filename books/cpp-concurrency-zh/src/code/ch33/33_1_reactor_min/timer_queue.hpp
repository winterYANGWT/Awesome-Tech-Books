#pragma once

#include <chrono>
#include <cstdint>
#include <functional>
#include <map>
#include <unordered_set>

// 最小定时器队列：按到期时间排序的 multimap，取消用惰性标记。
class TimerQueue {
public:
    using Clock = std::chrono::steady_clock;
    using Callback = std::function<void()>;

    std::uint64_t add(Clock::time_point when, Callback cb);
    void cancel(std::uint64_t id);

    // 距最近一个定时器还有多少毫秒；没有定时器返回 -1。
    int timeout_ms(Clock::time_point now) const;
    void run_expired(Clock::time_point now);

private:
    struct Entry {
        std::uint64_t id;
        Callback cb;
    };
    std::multimap<Clock::time_point, Entry> timers_;
    std::unordered_set<std::uint64_t> cancelled_;
    std::uint64_t next_id_ = 1;
};
