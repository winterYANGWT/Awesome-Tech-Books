// 请求队列不设上限：I/O 线程一直收，慢 worker 一直处理不完。
// mode: crash
// expect: 队列长度达到 10000
#include <cassert>
#include <chrono>
#include <condition_variable>
#include <cstdio>
#include <cstdlib>
#include <deque>
#include <mutex>
#include <thread>

namespace {

constexpr std::size_t kLimit = 10000;

std::mutex mu;
std::condition_variable not_empty;
std::deque<int> requests;

void slow_worker() {
    for (;;) {
        int request = 0;
        {
            std::unique_lock<std::mutex> lock(mu);
            not_empty.wait(lock, [] { return !requests.empty(); });
            request = requests.front();
            requests.pop_front();
        }
        (void)request;
        // 模拟业务处理：每 1 ms 才消费一个请求。
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
}

}  // namespace

int main() {
    std::thread worker(slow_worker);

    for (int i = 0;; ++i) {
        std::size_t depth = 0;
        {
            std::lock_guard<std::mutex> lock(mu);
            requests.push_back(i);
            depth = requests.size();
        }
        not_empty.notify_one();
        if (depth >= kLimit) {
            std::fprintf(stderr,
                         "队列长度达到 %zu：没有上限，慢客户端把内存和尾延迟一起拖垮\n",
                         depth);
            assert(false && "request queue has no limit");
        }
    }
}
