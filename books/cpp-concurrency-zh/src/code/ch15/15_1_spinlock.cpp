// mode: run
// expect: 结果: 400000
// expect: 同一时刻只有一个线程进入: 是
#include <atomic>
#include <cstdio>
#include <thread>
#include <vector>

// 没有任何阻塞调用：lock() 只是反复 test_and_set。
class SpinLock {
public:
    void lock() {
        while (flag_.test_and_set(std::memory_order_acquire)) {
        }
    }
    void unlock() { flag_.clear(std::memory_order_release); }

private:
    std::atomic_flag flag_{};  // C++20 起默认构造就是 clear
};

SpinLock lock;
long counter = 0;          // 只在临界区里访问
int inside = 0;            // 只在临界区里访问：当前有几个线程在锁内
int max_inside = 0;        // 观察到的最大并发数

void worker() {
    for (int i = 0; i < 100'000; ++i) {
        lock.lock();
        ++counter;
        ++inside;
        if (inside > max_inside) {
            max_inside = inside;
        }
        --inside;
        lock.unlock();
    }
}

int main() {
    std::vector<std::thread> threads;
    for (int i = 0; i < 4; ++i) {
        threads.emplace_back(worker);
    }
    for (auto& t : threads) {
        t.join();
    }
    std::printf("结果: %ld\n", counter);
    std::printf("同一时刻只有一个线程进入: %s（最大并发数 %d）\n",
                max_inside == 1 ? "是" : "否", max_inside);
}
