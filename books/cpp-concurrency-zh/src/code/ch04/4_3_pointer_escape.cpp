// mode: race
// expect: data race
// process_data 在锁内把内部 vector 的地址交给回调，回调把它带到了锁外。
#include <atomic>
#include <iostream>
#include <mutex>
#include <thread>
#include <vector>

class Buffer {
public:
    template <class F>
    void process_data(F&& f) {
        std::lock_guard<std::mutex> lock(m_);
        f(data_);  // 引用离开这个作用域时，锁已经不起作用了
    }

private:
    std::mutex m_;
    std::vector<int> data_{0, 0, 0};
};

int main() {
    Buffer buf;
    int* escaped = nullptr;
    buf.process_data([&](std::vector<int>& data) { escaped = data.data(); });
    std::cout << "回调把内部缓冲区的地址存到了外面\n";

    std::atomic<bool> ready{false};
    std::atomic<bool> stop{false};
    std::thread worker([&] {
        ready.store(true, std::memory_order_relaxed);
        while (!stop.load(std::memory_order_relaxed)) {
            buf.process_data([](std::vector<int>& data) { data[0] = data[0] + 1; });
        }
    });

    while (!ready.load(std::memory_order_relaxed)) {
        std::this_thread::yield();
    }
    for (int i = 0; i < 100000; ++i) {
        escaped[0] = i;  // 锁外写，和 worker 的锁内写撞在同一元素上
    }
    stop.store(true, std::memory_order_relaxed);
    worker.join();
    std::cout << "主线程在锁外写完了\n";
}
