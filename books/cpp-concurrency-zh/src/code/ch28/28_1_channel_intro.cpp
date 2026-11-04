// mode: run
// 同一个问题改成消息传递：发送者把 unique_ptr<Message> 移进通道，接收者取走所有权。
#include <atomic>
#include <condition_variable>
#include <cstdio>
#include <deque>
#include <memory>
#include <mutex>
#include <optional>
#include <thread>
#include <utility>

struct Message {
    int producer;
    int seq;

    Message(int p, int s) : producer(p), seq(s) { ++live; }
    Message(const Message&) = delete;
    Message& operator=(const Message&) = delete;
    ~Message() { --live; }

    static inline std::atomic<int> live{0};
};

// 最小通道：只保证 FIFO 和关闭，不限制容量。
class Channel {
public:
    bool send(std::unique_ptr<Message> m) {
        {
            std::lock_guard<std::mutex> lock(m_);
            if (closed_) {
                return false;
            }
            q_.push_back(std::move(m));
        }
        not_empty_.notify_one();
        return true;
    }

    std::optional<std::unique_ptr<Message>> recv() {
        std::unique_lock<std::mutex> lock(m_);
        not_empty_.wait(lock, [&] { return !q_.empty() || closed_; });
        if (q_.empty()) {
            return std::nullopt;
        }
        auto m = std::move(q_.front());
        q_.pop_front();
        return m;
    }

    void close() {
        {
            std::lock_guard<std::mutex> lock(m_);
            closed_ = true;
        }
        not_empty_.notify_all();
    }

private:
    std::mutex m_;
    std::condition_variable not_empty_;
    std::deque<std::unique_ptr<Message>> q_;
    bool closed_ = false;
};

int main() {
    constexpr int kPerProducer = 300;
    constexpr int kProducers = 2;
    constexpr int kConsumers = 2;

    Channel channel;
    std::thread producers[kProducers];
    for (int p = 0; p < kProducers; ++p) {
        producers[p] = std::thread([&channel, p] {
            for (int i = 0; i < kPerProducer; ++i) {
                channel.send(std::make_unique<Message>(p, i));
            }
        });
    }

    int count[kConsumers] = {};
    long sum[kConsumers] = {};
    int out_of_order[kConsumers] = {};
    std::thread consumers[kConsumers];
    for (int c = 0; c < kConsumers; ++c) {
        consumers[c] = std::thread([&, c] {
            int last[kProducers] = {-1, -1};
            while (auto m = channel.recv()) {
                ++count[c];
                sum[c] += (*m)->seq;
                if ((*m)->seq <= last[(*m)->producer]) {
                    ++out_of_order[c];
                }
                last[(*m)->producer] = (*m)->seq;
                // unique_ptr 在这里析构，Message 随之销毁。
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
    std::printf("发送者移入 %d 条，接收者取走 %d 条，校验和 %ld，乱序 %d 次\n",
                kProducers * kPerProducer, total, checksum, violations);
    std::printf("最后存活的消息对象：%d 个\n", Message::live.load());
}
