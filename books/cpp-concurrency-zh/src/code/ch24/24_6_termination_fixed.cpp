// mode: run
// expect: processed=8 sum=36 expected=8
#include <atomic>
#include <condition_variable>
#include <cstdio>
#include <mutex>
#include <queue>
#include <thread>

std::mutex m;
std::condition_variable cv;
std::queue<int> q;
bool closed = false;
std::atomic<bool> checked{false};
std::atomic<bool> go{false};
std::atomic<int> processed{0};
std::atomic<int> sum{0};

static void worker(int id) {
    // 复现同一个窗口：快速检查在锁外发生，检查完通知才到。
    if (id == 0 && q.empty()) {
        checked.store(true);
        while (!go.load()) std::this_thread::yield();
    }
    // ANCHOR: pred
    for (;;) {
        std::unique_lock<std::mutex> lk(m);
        // 谓词在锁内重新检查，任务已经在队列里就直接取走。
        cv.wait(lk, [] { return !q.empty() || closed; });
        if (q.empty() && closed) return;
        int v = q.front();
        q.pop();
        lk.unlock();
        processed.fetch_add(1, std::memory_order_relaxed);
        sum.fetch_add(v, std::memory_order_relaxed);
    }
    // ANCHOR_END: pred
}

int main() {
    std::thread w0(worker, 0);
    std::thread w1(worker, 1);
    while (!checked.load()) std::this_thread::yield();
    {
        std::lock_guard<std::mutex> lk(m);
        for (int v = 1; v <= 8; ++v) q.push(v);
        closed = true;
    }
    cv.notify_all();
    go.store(true);
    w0.join();
    w1.join();
    std::printf("processed=%d sum=%d expected=8\n", processed.load(), sum.load());
    return 0;
}
