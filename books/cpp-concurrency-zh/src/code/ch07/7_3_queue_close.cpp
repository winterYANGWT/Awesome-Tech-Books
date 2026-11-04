// mode: run
// expect: close 后 push 被拒绝 1 次\n关闭后取到 1000 个，丢弃 0 个
// 生产者提交 1000 个任务后关闭；消费者在关闭后才开始，把剩余任务取完再退出。
#include <condition_variable>
#include <cstdio>
#include <mutex>
#include <optional>
#include <queue>
#include <thread>
#include <utility>

namespace {

template <class T>
class ThreadSafeQueue {
public:
    bool push(T value) {
        {
            std::lock_guard<std::mutex> lock(m_);
            if (closed_) {
                return false;  // 关闭后不悄悄丢数据，交给调用者处理
            }
            q_.push(std::move(value));
        }
        cv_.notify_one();
        return true;
    }

// ANCHOR: close
    std::optional<T> wait_pop() {
        std::unique_lock<std::mutex> lock(m_);
        cv_.wait(lock, [&] { return !q_.empty() || closed_; });
        if (q_.empty()) {
            return std::nullopt;
        }
        T item = std::move(q_.front());
        q_.pop();
        return item;
    }

    void close() {
        {
            std::lock_guard<std::mutex> lock(m_);
            if (closed_) {
                return;  // 幂等：重复关闭不再通知一遍
            }
            closed_ = true;
        }
        cv_.notify_all();
    }
// ANCHOR_END: close

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

}  // namespace

int main() {
    ThreadSafeQueue<int> q;
    for (int i = 0; i < 1000; ++i) {
        if (!q.push(i)) {
            return 1;
        }
    }
    q.close();
    q.close();  // 重复调用是允许的

    const bool accepted = q.push(9999);
    std::printf("close 后 push 被拒绝 %d 次\n", accepted ? 0 : 1);

    int taken = 0;
    int dropped = 0;
    std::thread consumer([&] {
        while (auto value = q.wait_pop()) {
            (void)value;
            ++taken;
        }
        dropped = static_cast<int>(q.size());
    });
    consumer.join();

    std::printf("关闭后取到 %d 个，丢弃 %d 个\n", taken, dropped);
    return 0;
}
