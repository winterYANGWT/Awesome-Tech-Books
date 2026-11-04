// mode: run
// output: varies
// timeout: 60
// expect: 1000 轮：通过 1000，失败 0
// 对第 7 章的线程安全队列做随机 push/pop/close 压力测试。每轮用不同的
// 随机种子；失败时把种子和最近的操作序列打出来，下一轮可以用同一种子重放。
#include <atomic>
#include <chrono>
#include <condition_variable>
#include <cstdio>
#include <latch>
#include <mutex>
#include <optional>
#include <queue>
#include <random>
#include <string>
#include <thread>
#include <utility>
#include <vector>

namespace {

constexpr int kRounds = 1000;
constexpr int kProducers = 2;
constexpr int kConsumers = 2;
constexpr int kOpsPerProducer = 50;
constexpr unsigned kBaseSeed = 20261103U;

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

struct RoundResult {
    bool ok = true;
    int popped = 0;
    long long sum = 0;
};

// ANCHOR: round
RoundResult run_round(unsigned seed, int round) {
    Queue q;
    std::latch go{kProducers + kConsumers};
    std::atomic<int> producers_left{kProducers};
    std::vector<std::string> logs[kProducers];
    std::atomic<int> popped{0};
    std::atomic<long long> sum{0};

    std::vector<std::thread> threads;
    for (int p = 0; p < kProducers; ++p) {
        threads.emplace_back([&, p] {
            std::mt19937 rng(seed + static_cast<unsigned>(p) * 7919U);
            go.count_down();
            go.wait();
            for (int step = 0; step < kOpsPerProducer; ++step) {
                const int value = p * 100000 + step;
                q.push(value);
                logs[p].push_back(std::to_string(value));
                if (rng() % 10 == 0) {
                    std::this_thread::yield();
                }
                if (rng() % 16 == 0) {
                    std::this_thread::sleep_for(std::chrono::microseconds(20));
                }
            }
            if (producers_left.fetch_sub(1) == 1) {
                q.close();
            }
        });
    }
    for (int c = 0; c < kConsumers; ++c) {
        threads.emplace_back([&] {
            go.count_down();
            go.wait();
            long long local_sum = 0;
            int local_count = 0;
            while (auto value = q.wait_pop()) {
                local_sum += *value;
                ++local_count;
            }
            sum.fetch_add(local_sum, std::memory_order_relaxed);
            popped.fetch_add(local_count, std::memory_order_relaxed);
        });
    }

    for (std::thread& t : threads) {
        t.join();
    }

    RoundResult result;
    result.popped = popped.load();
    result.sum = sum.load();
    const int expected_count = kProducers * kOpsPerProducer;
    long long expected_sum = 0;
    for (int p = 0; p < kProducers; ++p) {
        for (int step = 0; step < kOpsPerProducer; ++step) {
            expected_sum += p * 100000 + step;
        }
    }
    if (result.popped != expected_count || result.sum != expected_sum) {
        result.ok = false;
        std::printf("第 %d 轮失败，种子 %u，线程 %d：取到 %d 个（应有 %d），"
                    "和 %lld（应为 %lld）\n",
                    round, seed, kProducers + kConsumers, result.popped,
                    expected_count, result.sum, expected_sum);
        for (int p = 0; p < kProducers; ++p) {
            std::printf("生产者 %d 最近的操作：", p);
            const std::size_t start =
                logs[p].size() > 10 ? logs[p].size() - 10 : 0;
            for (std::size_t i = start; i < logs[p].size(); ++i) {
                std::printf(" %s", logs[p][i].c_str());
            }
            std::printf("\n");
        }
    }
    return result;
}
// ANCHOR_END: round

}  // namespace

int main(int argc, char** argv) {
    unsigned base = kBaseSeed;
    int rounds = kRounds;
    for (int i = 1; i < argc; ++i) {
        const std::string arg = argv[i];
        if (arg == "--seed" && i + 1 < argc) {
            base = static_cast<unsigned>(std::stoul(argv[++i]));
        } else if (arg == "--rounds" && i + 1 < argc) {
            rounds = std::stoi(argv[++i]);
        }
    }

    int passed = 0;
    for (int round = 0; round < rounds; ++round) {
        const unsigned seed = base + static_cast<unsigned>(round);
        if (!run_round(seed, round).ok) {
            return 1;
        }
        ++passed;
    }
    std::printf("%d 轮：通过 %d，失败 0（种子起点 %u，线程 %d）\n", rounds,
                passed, base, kProducers + kConsumers);
    return 0;
}
