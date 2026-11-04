#include "timer_awaiter.hpp"

#include <queue>
#include <thread>
#include <vector>

namespace {

struct entry {
    std::chrono::steady_clock::time_point when;
    std::coroutine_handle<> handle;
};

struct later {
    bool operator()(const entry& a, const entry& b) const { return a.when > b.when; }
};

std::priority_queue<entry, std::vector<entry>, later>& queue() {
    static std::priority_queue<entry, std::vector<entry>, later> q;
    return q;
}

}  // namespace

// ANCHOR: suspend
void timer_awaiter::await_suspend(std::coroutine_handle<> handle) {
    // delay 已经拷进这个 awaiter；队列只存到期时间，不存指向 awaiter 的引用。
    queue().push({std::chrono::steady_clock::now() + delay, handle});
}

// ANCHOR_END: suspend

bool run_one_timer() {
    if (queue().empty()) return false;
    entry e = queue().top();
    queue().pop();
    std::this_thread::sleep_until(e.when);
    e.handle.resume();
    return true;
}
