// mode: hang
// expect: 仍未结束
#include <atomic>
#include <chrono>
#include <functional>
#include <iostream>
#include <mutex>
#include <thread>

// 两个线程每次都先拿自己那一把，再同时试另一把；失败后立即重来。
// 下面的自旋屏障把两边固定在“各持一把”的阶段，演示活锁而不是靠运气。
class SpinBarrier {
public:
    explicit SpinBarrier(int total) : total_(total) {}

    void wait() {
        unsigned gen = generation_.load(std::memory_order_acquire);
        if (count_.fetch_add(1, std::memory_order_acq_rel) + 1 == total_) {
            count_.store(0, std::memory_order_relaxed);
            generation_.fetch_add(1, std::memory_order_release);
            return;
        }
        while (generation_.load(std::memory_order_acquire) == gen) {
            std::this_thread::yield();
        }
    }

private:
    int total_;
    std::atomic<int> count_{0};
    std::atomic<unsigned> generation_{0};
};

std::mutex m1, m2;
std::atomic<unsigned long> attempts{0};
std::atomic<unsigned long> completed{0};

void worker(std::mutex& first, std::mutex& second, SpinBarrier& barrier) {
    for (;;) {
        first.lock();
        barrier.wait();  // 两边都拿到第一把，才同时去试第二把

        if (second.try_lock()) {  // 正常情况下永远失败：第二把在对方手里
            completed.fetch_add(1, std::memory_order_relaxed);
            second.unlock();
        }
        barrier.wait();  // 两边都试过第二把，才允许释放第一把
        first.unlock();
        barrier.wait();  // 两边都释放后，立即开始下一轮，没有退避
        attempts.fetch_add(1, std::memory_order_relaxed);
    }
}

int main() {
    SpinBarrier barrier(2);
    std::thread a(worker, std::ref(m1), std::ref(m2), std::ref(barrier));
    std::thread b(worker, std::ref(m2), std::ref(m1), std::ref(barrier));

    for (;;) {
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
        std::cout << "完成 " << completed.load() << " 次，重试 "
                  << attempts.load() << " 次\n" << std::flush;
    }
}
