// mode: run
// output: varies
// tsan: no（计时示例，TSan 会放大线程创建和同步的开销）
// expect: mode,ms\nper_task,\d+\.\d+\npool,\d+\.\d+
// 每个任务一个线程 vs 固定池：任务本身不到 1 微秒，差别主要在创建和销毁。
#include <algorithm>
#include <array>
#include <atomic>
#include <chrono>
#include <condition_variable>
#include <cstdint>
#include <cstdio>
#include <functional>
#include <mutex>
#include <queue>
#include <thread>
#include <utility>
#include <vector>

namespace {

using Clock = std::chrono::steady_clock;

constexpr int kTasks = 400;
constexpr int kWorkers = 8;
constexpr int kRounds = 3;

std::atomic<std::uint64_t> g_sink{0};

std::uint64_t work(int seed) {
    std::uint64_t x = 1469598103934665603ULL ^ static_cast<std::uint64_t>(seed);
    for (int i = 0; i < 500; ++i) {
        x = (x ^ static_cast<std::uint64_t>(i)) * 1099511628211ULL;
    }
    return x;
}

class FixedPool {
public:
    explicit FixedPool(int n) {
        for (int i = 0; i < n; ++i) {
            workers_.emplace_back([this] { worker(); });
        }
    }

    ~FixedPool() { shutdown(); }

    void submit(std::function<void()> task) {
        {
            std::lock_guard<std::mutex> lock(m_);
            if (done_) {
                return;
            }
            q_.push(std::move(task));
        }
        cv_.notify_one();
    }

    void shutdown() {
        {
            std::lock_guard<std::mutex> lock(m_);
            done_ = true;
        }
        cv_.notify_all();
        for (auto& t : workers_) {
            if (t.joinable()) {
                t.join();
            }
        }
    }

private:
    void worker() {
        for (;;) {
            std::function<void()> task;
            {
                std::unique_lock<std::mutex> lock(m_);
                cv_.wait(lock, [this] { return done_ || !q_.empty(); });
                if (q_.empty()) {
                    return;
                }
                task = std::move(q_.front());
                q_.pop();
            }
            try {
                task();
            } catch (...) {
                // 任务异常不能逃出工作线程；这里只记下数目。
                failed_.fetch_add(1, std::memory_order_relaxed);
            }
        }
    }

    std::mutex m_;
    std::condition_variable cv_;
    std::queue<std::function<void()>> q_;
    std::vector<std::thread> workers_;
    bool done_ = false;
    std::atomic<int> failed_{0};
};

double per_task_once() {
    auto t0 = Clock::now();
    for (int i = 0; i < kTasks; ++i) {
        std::thread t([i] { g_sink.fetch_add(work(i), std::memory_order_relaxed); });
        t.join();
    }
    auto t1 = Clock::now();
    return std::chrono::duration<double, std::milli>(t1 - t0).count();
}

double pool_once() {
    FixedPool pool(kWorkers);
    auto t0 = Clock::now();
    for (int i = 0; i < kTasks; ++i) {
        pool.submit([i] { g_sink.fetch_add(work(i), std::memory_order_relaxed); });
    }
    pool.shutdown();
    auto t1 = Clock::now();
    return std::chrono::duration<double, std::milli>(t1 - t0).count();
}

template <class F>
double median_ms(F&& f) {
    std::array<double, kRounds> samples{};
    for (auto& s : samples) {
        s = f();
    }
    std::sort(samples.begin(), samples.end());
    return samples[kRounds / 2];
}

}  // namespace

int main() {
    per_task_once();  // 预热
    pool_once();
    const double per_task = median_ms(per_task_once);
    const double pool = median_ms(pool_once);
    std::printf("mode,ms\nper_task,%.3f\npool,%.3f\n", per_task, pool);
    return 0;
}
