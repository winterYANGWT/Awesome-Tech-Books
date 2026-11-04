// mode: run
// expect: 1000 轮全部通过，共取到 200000 个元素
// 起跑门：两个线程先到 latch 报到，主线程放开 go，再同时开始 push/pop。
// 不是用 sleep 猜“大概同时”，而是让两个线程都卡在同一个点上。
#include <condition_variable>
#include <cstdio>
#include <latch>
#include <mutex>
#include <optional>
#include <queue>
#include <thread>
#include <utility>

namespace {

constexpr int kRounds = 1000;
constexpr int kItems = 200;

class Queue {
public:
    void push(int value) {
        {
            std::lock_guard<std::mutex> lock(m_);
            q_.push(value);
        }
        cv_.notify_one();
    }

    std::optional<int> wait_pop() {
        std::unique_lock<std::mutex> lock(m_);
        cv_.wait(lock, [&] { return !q_.empty() || closed_; });
        if (q_.empty()) {
            return std::nullopt;
        }
        int value = q_.front();
        q_.pop();
        return value;
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

private:
    std::mutex m_;
    std::condition_variable cv_;
    std::queue<int> q_;
    bool closed_ = false;
};

}  // namespace

int main() {
    long long total_items = 0;
    // ANCHOR: round
    for (int round = 0; round < kRounds; ++round) {
        Queue q;
        std::latch ready{2};
        std::latch go{1};

        std::thread producer([&] {
            ready.count_down();
            go.wait();
            for (int i = 0; i < kItems; ++i) {
                q.push(i);
            }
            q.close();
        });

        long long sum = 0;
        int count = 0;
        std::thread consumer([&] {
            ready.count_down();
            go.wait();
            while (auto value = q.wait_pop()) {
                sum += *value;
                ++count;
            }
        });

        ready.wait();
        go.count_down();
        producer.join();
        consumer.join();

        const long long expected =
            static_cast<long long>(kItems) * (kItems - 1) / 2;
        if (count != kItems || sum != expected) {
            std::printf("第 %d 轮失败：取到 %d 个，和 %lld\n", round, count, sum);
            return 1;
        }
        total_items += count;
    }
    // ANCHOR_END: round
    std::printf("%d 轮全部通过，共取到 %lld 个元素\n", kRounds, total_items);
    return 0;
}
