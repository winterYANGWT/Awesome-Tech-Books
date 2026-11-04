// mode: run
// 毒丸：每个生产者发完自己的消息后放一个“停止”消息，消费者见到 3 个才退出。
#include <condition_variable>
#include <cstdio>
#include <deque>
#include <mutex>
#include <optional>
#include <thread>
#include <utility>
#include <variant>

struct data_msg {
    int producer;
    int seq;
};
struct pill {
    int producer;
};
using Msg = std::variant<data_msg, pill>;

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

    Channel<Msg> channel(16);
    std::thread producers[kProducers];
    for (int p = 0; p < kProducers; ++p) {
        producers[p] = std::thread([&channel, p] {
            for (int i = 0; i < kPerProducer; ++i) {
                channel.send(data_msg{p, i});
            }
            channel.send(pill{p});
        });
    }

    int data = 0;
    int pills = 0;
    long sum = 0;
    std::thread consumer([&] {
        while (auto msg = channel.recv()) {
            if (auto* d = std::get_if<data_msg>(&*msg)) {
                ++data;
                sum += d->seq;
            } else if (++pills == kProducers) {
                break;
            }
        }
    });

    for (auto& t : producers) {
        t.join();
    }
    consumer.join();
    channel.close();

    std::printf("数据 %d 条，毒丸 %d 个，校验和 %ld\n", data, pills, sum);
}
