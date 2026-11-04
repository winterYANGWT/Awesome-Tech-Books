// mode: hang
// output: varies
// timeout: 2
// expect: 仍未结束
#include <atomic>
#include <condition_variable>
#include <cstdio>
#include <mutex>
#include <queue>
#include <thread>

std::mutex m;
std::condition_variable cv;
std::queue<int> q;
std::atomic<bool> checked{false};
std::atomic<bool> go{false};

// ANCHOR: wait
static void worker() {
    // 错误写法：在锁外做一次快速检查，然后用不带谓词的 wait 睡下去。
    if (q.empty()) {
        std::printf("worker: 队列为空，准备等待\n");
        checked.store(true);
        // 为了稳定复现，等主线程把任务放进去、通知发完再继续。
        while (!go.load()) std::this_thread::yield();
        std::unique_lock<std::mutex> lk(m);
        cv.wait(lk);   // 通知已经发过，这里没有人再叫醒它
        if (!q.empty()) q.pop();
        std::printf("worker: 被唤醒\n");
    }
}
// ANCHOR_END: wait

int main() {
    std::thread t(worker);
    while (!checked.load()) std::this_thread::yield();
    {
        std::lock_guard<std::mutex> lk(m);
        q.push(42);
        std::printf("main: 任务已入队\n");
    }
    cv.notify_one();
    go.store(true);
    std::printf("main: 已经 notify_one，等待 worker\n");
    t.join();
    std::printf("main: worker 结束\n");
    return 0;
}
