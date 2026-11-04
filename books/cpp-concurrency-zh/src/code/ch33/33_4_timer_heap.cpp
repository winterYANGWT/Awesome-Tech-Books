// mode: run
// 最小堆定时器：std::priority_queue 默认是最大堆，这里显式给 std::greater。
#include <sys/epoll.h>
#include <unistd.h>

#include <chrono>
#include <cstdio>
#include <functional>
#include <queue>
#include <vector>

using Clock = std::chrono::steady_clock;

struct Timer {
    Clock::time_point due;
    int id;
    int offset_ms;
};

// 这里用 std::greater<Timer> 调用 operator>；due 早的排在堆顶。
bool operator>(const Timer &a, const Timer &b) { return a.due > b.due; }

int main() {
    const int offsets[10] = {350, 50, 500, 150, 250, 450, 100, 300, 200, 400};
    std::priority_queue<Timer, std::vector<Timer>, std::greater<Timer>> heap;
    const auto start = Clock::now();
    for (int i = 0; i < 10; ++i) {
        heap.push(Timer{start + std::chrono::milliseconds(offsets[i]), i + 1, offsets[i]});
    }

    const int ep = ::epoll_create1(EPOLL_CLOEXEC);
    epoll_event ev{};
    while (!heap.empty()) {
        const auto now = Clock::now();
        const auto left = heap.top().due - now;
        const int timeout = left <= Clock::duration::zero()
                                ? 0
                                : static_cast<int>(std::chrono::duration_cast<
                                                       std::chrono::milliseconds>(left)
                                                       .count()) +
                                      1;
        const int n = ::epoll_wait(ep, &ev, 1, timeout);
        if (n < 0) continue;  // EINTR
        const auto after = Clock::now();
        while (!heap.empty() && heap.top().due <= after) {
            const Timer t = heap.top();
            heap.pop();
            std::printf("fired id=%d due=%dms\n", t.id, t.offset_ms);
        }
    }
    ::close(ep);
    std::printf("all 10 fired\n");
    return 0;
}
