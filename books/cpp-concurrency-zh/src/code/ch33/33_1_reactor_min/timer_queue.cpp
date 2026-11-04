#include "timer_queue.hpp"

#include <algorithm>

std::uint64_t TimerQueue::add(Clock::time_point when, Callback cb) {
    const std::uint64_t id = next_id_++;
    timers_.emplace(when, Entry{id, std::move(cb)});
    return id;
}

void TimerQueue::cancel(std::uint64_t id) {
    if (id != 0) cancelled_.insert(id);
}

int TimerQueue::timeout_ms(Clock::time_point now) const {
    if (timers_.empty()) return -1;
    const auto left = timers_.begin()->first - now;
    if (left <= Clock::duration::zero()) return 0;
    const auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(left);
    return static_cast<int>(std::max<std::int64_t>(1, ms.count()));
}

void TimerQueue::run_expired(Clock::time_point now) {
    while (!timers_.empty() && timers_.begin()->first <= now) {
        auto it = timers_.begin();
        Entry entry = std::move(it->second);
        timers_.erase(it);
        if (cancelled_.erase(entry.id) != 0) continue;
        entry.cb();  // 回调里可以再 add/cancel
    }
}
