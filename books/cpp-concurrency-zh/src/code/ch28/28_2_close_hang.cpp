// mode: hang
// expect: 仍未结束
// timeout: 2
// 反例：close() 只置标志、不唤醒，消费者永远睡在 not_empty 上。
#include <atomic>
#include <chrono>
#include <condition_variable>
#include <cstdio>
#include <deque>
#include <mutex>
#include <optional>
#include <thread>
#include <utility>

template <class T>
class Channel {
public:
    explicit Channel(std::size_t capacity) : capacity_(capacity) {}

    bool send(T value) {
        std::unique_lock<std::mutex> lock(m_);
        not_full_.wait(lock, [&] { return q_.size() < capacity_ || closed_; });
        if (closed_) {
            return false;
        }
        q_.push_back(std::move(value));
        lock.unlock();
        not_empty_.notify_one();
        return true;
    }

    std::optional<T> recv() {
        std::unique_lock<std::mutex> lock(m_);
        not_empty_.wait(lock, [&] { return !q_.empty() || closed_; });
        if (q_.empty()) {
            return std::nullopt;
        }
        T value = std::move(q_.front());
        q_.pop_front();
        lock.unlock();
        not_full_.notify_one();
        return value;
    }

    // ANCHOR: close
    void close() {
        // 错误：只置标志，没有 notify_all。已经睡在 not_empty_ 上的消费者不会醒。
        std::lock_guard<std::mutex> lock(m_);
        closed_ = true;
    }
    // ANCHOR_END: close

private:
    std::mutex m_;
    std::condition_variable not_empty_;
    std::condition_variable not_full_;
    std::deque<T> q_;
    std::size_t capacity_;
    bool closed_ = false;
};

int main() {
    constexpr int kPerProducer = 100;
    constexpr int kProducers = 4;
    constexpr int kConsumers = 4;

    Channel<int> channel(8);
    std::thread producers[kProducers];
    for (int p = 0; p < kProducers; ++p) {
        producers[p] = std::thread([&channel, p] {
            for (int i = 0; i < kPerProducer; ++i) {
                channel.send(p * kPerProducer + i);
            }
        });
    }

    std::atomic<int> consumed{0};
    std::thread consumers[kConsumers];
    for (auto& t : consumers) {
        t = std::thread([&] {
            while (channel.recv().has_value()) {
                ++consumed;
            }
        });
    }

    for (auto& t : producers) {
        t.join();
    }
    while (consumed.load() < kProducers * kPerProducer) {
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
    // 为了稳定复现：等所有消费者都回到 recv 里等待，再调用有问题的 close。
    std::this_thread::sleep_for(std::chrono::milliseconds(200));
    std::printf("%d 条全部取完，%d 个消费者回到 not_empty 等待\n",
                kProducers * kPerProducer, kConsumers);
    std::printf("close() 只置 closed，没有 notify_all\n");
    std::fflush(stdout);
    channel.close();
    for (auto& t : consumers) {
        t.join();
    }
}
