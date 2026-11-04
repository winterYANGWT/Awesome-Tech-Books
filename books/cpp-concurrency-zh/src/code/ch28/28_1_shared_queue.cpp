// mode: run
// 最小共享队列：一个 std::queue，一把 mutex，一个 condition_variable。
#include <condition_variable>
#include <cstdio>
#include <mutex>
#include <optional>
#include <queue>
#include <thread>
#include <utility>

struct Message {
    int producer;
    int seq;
};

class SharedQueue {
public:
    void push(Message m) {
        {
            std::lock_guard<std::mutex> lock(m_);
            q_.push(std::move(m));
        }
        not_empty_.notify_one();
    }

    std::optional<Message> pop() {
        std::unique_lock<std::mutex> lock(m_);
        not_empty_.wait(lock, [&] { return !q_.empty() || closed_; });
        if (q_.empty()) {
            return std::nullopt;
        }
        Message m = std::move(q_.front());
        q_.pop();
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
    std::queue<Message> q_;
    bool closed_ = false;
};

int main() {
    constexpr int kPerProducer = 500;
    constexpr int kProducers = 2;
    constexpr int kConsumers = 2;

    SharedQueue queue;
    std::thread producers[kProducers];
    for (int p = 0; p < kProducers; ++p) {
        producers[p] = std::thread([&queue, p] {
            for (int i = 0; i < kPerProducer; ++i) {
                queue.push(Message{p, i});
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
            while (auto m = queue.pop()) {
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
    queue.close();
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
    std::printf("生产者入队 %d 条\n", kProducers * kPerProducer);
    std::printf("消费者取到 %d 条，校验和 %ld，每个生产者的子序列乱序 %d 次\n",
                total, checksum, violations);
}
