// mode: run
// expect: 入队 100000，出队 100000，校验和 4999950000
// 4 个生产者、4 个消费者，共 10 万个任务；主线程在生产者结束后 close。
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

// ANCHOR: queue
template <class T>
class ThreadSafeQueue {
public:
    bool push(T value) {
        {
            std::lock_guard<std::mutex> lock(m_);
            if (closed_) {
                return false;
            }
            q_.push(std::move(value));
        }
        cv_.notify_one();
        return true;
    }

    bool try_pop(T& out) {
        std::lock_guard<std::mutex> lock(m_);
        if (q_.empty()) {
            return false;
        }
        out = std::move(q_.front());
        q_.pop();
        return true;
    }

    std::optional<T> wait_pop() {
        std::unique_lock<std::mutex> lock(m_);
        cv_.wait(lock, [&] { return !q_.empty() || closed_; });
        if (q_.empty()) {
            return std::nullopt;  // 已关闭且取空
        }
        T item = std::move(q_.front());
        q_.pop();
        return item;
    }

    void close() {
        {
            std::lock_guard<std::mutex> lock(m_);
            if (closed_) {
                return;
            }
            closed_ = true;
        }
        cv_.notify_all();
    }

    std::size_t size() const {
        std::lock_guard<std::mutex> lock(m_);
        return q_.size();
    }

private:
    mutable std::mutex m_;
    std::condition_variable cv_;
    std::queue<T> q_;
    bool closed_ = false;
};
// ANCHOR_END: queue

struct Task {
    int id;
    std::uint64_t value;
};

constexpr int kProducers = 4;
constexpr int kConsumers = 4;
constexpr int kTasks = 100000;

}  // namespace

int main() {
    ThreadSafeQueue<Task> q;
    std::vector<std::thread> producers;
    std::vector<std::thread> consumers;
    std::atomic<int> pushed{0};
    std::atomic<std::uint64_t> checksum{0};
    std::atomic<int> popped{0};

    for (int p = 0; p < kProducers; ++p) {
        producers.emplace_back([&, p] {
            for (int i = p; i < kTasks; i += kProducers) {
                if (q.push(Task{i, static_cast<std::uint64_t>(i)})) {
                    pushed.fetch_add(1, std::memory_order_relaxed);
                }
            }
        });
    }
    for (int c = 0; c < kConsumers; ++c) {
        consumers.emplace_back([&] {
            std::uint64_t sum = 0;
            int n = 0;
            while (auto task = q.wait_pop()) {
                sum += task->value;
                ++n;
            }
            checksum.fetch_add(sum, std::memory_order_relaxed);
            popped.fetch_add(n, std::memory_order_relaxed);
        });
    }

    for (auto& t : producers) {
        t.join();
    }
    q.close();
    for (auto& t : consumers) {
        t.join();
    }

    std::printf("入队 %d，出队 %d，校验和 %llu\n", pushed.load(), popped.load(),
                static_cast<unsigned long long>(checksum.load()));
    return 0;
}
