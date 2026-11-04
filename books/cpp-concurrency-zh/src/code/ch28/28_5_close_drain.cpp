// mode: run
// close 的排空语义：不再收新消息，已入队的消息由 recv 取完，最后返回 nullopt。
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

    void close() {
        {
            std::lock_guard<std::mutex> lock(m_);
            closed_ = true;
        }
        not_empty_.notify_all();
        not_full_.notify_all();
    }

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
    constexpr int kProducers = 3;

    Channel<int> channel(8);
    std::thread producers[kProducers];
    for (int p = 0; p < kProducers; ++p) {
        producers[p] = std::thread([&channel, p] {
            for (int i = 0; i < kPerProducer; ++i) {
                channel.send(p * kPerProducer + i);
            }
        });
    }

    int count = 0;
    long sum = 0;
    std::thread consumer([&] {
        while (auto value = channel.recv()) {
            ++count;
            sum += *value;
        }
        std::printf("取到 %d 条后收到 nullopt，校验和 %ld\n", count, sum);
    });

    for (auto& t : producers) {
        t.join();
    }
    channel.close();
    consumer.join();
}
