// mode: run
// expect: 出队 10000，校验和 50005000
// 一个生产者、两个消费者；close() 之后消费者取空剩余任务再退出。
#include <atomic>
#include <condition_variable>
#include <cstdint>
#include <cstdio>
#include <mutex>
#include <optional>
#include <queue>
#include <thread>
#include <utility>
#include <vector>

namespace {

template <class T>
class SingleLockQueue {
public:
    void push(T value) {
        {
            std::lock_guard<std::mutex> lock(m_);
            queue_.push(std::move(value));
        }
        cv_.notify_one();  // 先解锁再通知：被唤醒的线程不用立刻撞回互斥量
    }

    std::optional<T> try_pop() {
        std::lock_guard<std::mutex> lock(m_);
        if (queue_.empty()) {
            return std::nullopt;  // 空队列不抛异常
        }
        T value = std::move(queue_.front());
        queue_.pop();
        return value;
    }

    std::optional<T> wait_pop() {
        std::unique_lock<std::mutex> lock(m_);
        cv_.wait(lock, [&] { return !queue_.empty() || closed_; });
        if (queue_.empty()) {
            return std::nullopt;
        }
        T value = std::move(queue_.front());
        queue_.pop();
        return value;
    }

    void close() {
        {
            std::lock_guard<std::mutex> lock(m_);
            closed_ = true;
        }
        cv_.notify_all();
    }

private:
    std::mutex m_;
    std::condition_variable cv_;
    std::queue<T> queue_;
    bool closed_ = false;
};

}  // namespace

int main() {
    constexpr int kItems = 10000;
    SingleLockQueue<int> queue;
    std::atomic<int> popped{0};
    std::atomic<std::uint64_t> checksum{0};

    std::thread producer([&] {
        for (int i = 1; i <= kItems; ++i) {
            queue.push(i);
        }
    });

    std::vector<std::thread> consumers;
    for (int c = 0; c < 2; ++c) {
        consumers.emplace_back([&] {
            std::uint64_t sum = 0;
            int n = 0;
            while (auto value = queue.wait_pop()) {
                sum += static_cast<std::uint64_t>(*value);
                ++n;
            }
            checksum.fetch_add(sum, std::memory_order_relaxed);
            popped.fetch_add(n, std::memory_order_relaxed);
        });
    }

    producer.join();
    queue.close();
    for (auto& thread : consumers) {
        thread.join();
    }

    std::printf("出队 %d，校验和 %llu\n", popped.load(),
                static_cast<unsigned long long>(checksum.load()));
    return 0;
}
