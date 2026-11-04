// mode: run
// 有界通道：4 个生产者、4 个消费者，容量 8；生产者结束后 close，消费者排空后退出。
#include <atomic>
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

struct Message {
    int producer;
    int seq;
};

int main() {
    constexpr int kPerProducer = 250;
    constexpr int kProducers = 4;
    constexpr int kConsumers = 4;

    Channel<Message> channel(8);
    std::atomic<int> sent{0};
    std::thread producers[kProducers];
    for (int p = 0; p < kProducers; ++p) {
        producers[p] = std::thread([&, p] {
            for (int i = 0; i < kPerProducer; ++i) {
                if (channel.send(Message{p, i})) {
                    ++sent;
                }
            }
        });
    }

    int count[kConsumers] = {};
    long sum[kConsumers] = {};
    int out_of_order[kConsumers] = {};
    std::thread consumers[kConsumers];
    for (int c = 0; c < kConsumers; ++c) {
        consumers[c] = std::thread([&, c] {
            int last[kProducers] = {-1, -1, -1, -1};
            while (auto m = channel.recv()) {
                ++count[c];
                sum[c] += m->seq;
                if (m->seq <= last[m->producer]) {
                    ++out_of_order[c];
                }
                last[m->producer] = m->seq;
            }
        });
    }

    for (auto& t : producers) {
        t.join();
    }
    channel.close();
    for (auto& t : consumers) {
        t.join();
    }

    int total = 0;
    long checksum = 0;
    int violations = 0;
    for (int c = 0; c < kConsumers; ++c) {
        total += count[c];
        checksum += sum[c];
        violations += out_of_order[c];
    }
    std::printf("容量 8：入队 %d 条，取到 %d 条\n", sent.load(), total);
    std::printf("校验和 %ld，每个消费者看到的子序列乱序 %d 次\n", checksum, violations);
}
