// mode: run
// expect: if 版本错误 200 次；while 版本错误 0 次（各 200 轮）
// 在消费者已经睡进 wait 之后，另一个线程只为“没有满足谓词”的条件发一次通知。
// if 版本把这次返回当成“队列非空”，while 版本重新检查谓词，把这次唤醒消化掉。
#include <atomic>
#include <condition_variable>
#include <cstdio>
#include <mutex>
#include <thread>

namespace {

constexpr int kRounds = 200;

// ANCHOR: round
// 返回“谓词为假却从 wait 返回”的次数。
int run_round(bool use_while) {
    std::mutex m;
    std::condition_variable cv;
    bool has_item = false;
    std::atomic<int> stage{0};
    std::atomic<int> woke{0};
    int errors = 0;

    std::thread consumer([&] {
        std::unique_lock<std::mutex> lock(m);
        stage.store(1, std::memory_order_release);
        if (use_while) {
            while (!has_item) {
                cv.wait(lock);
            }
        } else {
            if (!has_item) {
                cv.wait(lock);  // 故意只等一次
            }
            if (!has_item) {
                ++errors;  // 谓词仍为假，却把空队列当成有元素
                woke.store(1, std::memory_order_release);
            }
            while (!has_item) {
                cv.wait(lock);  // 等真正的那次通知
            }
        }
    });

    // 等消费者把 stage 置 1；它随后调用 wait 并释放 mutex。
    while (stage.load(std::memory_order_acquire) != 1) {
        std::this_thread::yield();
    }

    {
        // 能拿到锁，说明消费者已经进入 wait；这次 notify 一定会把它唤醒。
        std::unique_lock<std::mutex> lock(m);
        cv.notify_one();
    }

    if (!use_while) {
        // 等 if 版本先处理完这次假唤醒，再放入真正的元素。
        while (woke.load(std::memory_order_acquire) == 0) {
            std::this_thread::yield();
        }
    }

    {
        std::lock_guard<std::mutex> lock(m);
        has_item = true;
    }
    cv.notify_one();
    consumer.join();
    return errors;
}
// ANCHOR_END: round

}  // namespace

int main() {
    int bad_if = 0;
    int bad_while = 0;
    for (int i = 0; i < kRounds; ++i) {
        bad_if += run_round(false);
        bad_while += run_round(true);
    }
    std::printf("if 版本错误 %d 次；while 版本错误 %d 次（各 %d 轮）\n",
                bad_if, bad_while, kRounds);
    return 0;
}
