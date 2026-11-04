// mode: hang
// flags: -fno-inline
// 关掉内联，让 gdb 栈保留函数帧。
// expect: pthread_cond_wait|仍未结束
// 消费者检查到队列为空，生产者在这之后、消费者真正进入 wait 之前通知。
// 用两个 latch 把这个窗口固定下来：通知发出时没有等待者，条件变量不会记住它。
#include <condition_variable>
#include <cstdio>
#include <latch>
#include <mutex>
#include <queue>
#include <thread>

namespace {

std::mutex m;
std::condition_variable cv;
std::queue<int> q;
std::latch checked{1};
std::latch notified{1};

}  // namespace

int main() {
    std::thread consumer([] {
        std::unique_lock<std::mutex> lock(m);
        if (q.empty()) {
            // 已经确认队列为空；先放开锁，让生产者完成 push + notify。
            lock.unlock();
            checked.count_down();
            notified.wait();
            lock.lock();
            // 这里没有重新检查谓词，直接睡；通知已经发过了。
            cv.wait(lock);
        }
        std::printf("消费者取到 %d\n", q.front());
    });

    checked.wait();
    {
        std::lock_guard<std::mutex> lock(m);
        q.push(42);
    }
    std::printf("生产者 push 后 notify_one（此时没有等待者）\n");
    std::fflush(stdout);
    cv.notify_one();
    notified.count_down();

    consumer.join();
    std::printf("不会走到这里\n");
}
