// mode: run
// output: varies
// tsan: no（计时示例，TSan 会改变同步开销）
// expect: scheme,ms,p99_ms\nglobal,\d+\.\d+,\d+\.\d+\nlocal,\d+\.\d+,\d+\.\d+\n# checksum \d+
// 同一组任务：一个全局队列 vs 每个工作线程一个本地队列。
#include <algorithm>
#include <array>
#include <atomic>
#include <barrier>
#include <chrono>
#include <condition_variable>
#include <cstdint>
#include <cstdio>
#include <functional>
#include <latch>
#include <mutex>
#include <queue>
#include <thread>
#include <utility>
#include <vector>

namespace {

using Clock = std::chrono::steady_clock;

constexpr int kWorkers = 4;
constexpr int kRoots = 4;
constexpr int kChildren = 2000;
constexpr int kIters = 2000;
constexpr int kRounds = 3;

std::uint64_t work(int iters) {
    std::uint64_t x = 1469598103934665603ULL;
    for (int i = 0; i < iters; ++i) {
        x = (x ^ static_cast<std::uint64_t>(i)) * 1099511628211ULL;
    }
    return x;
}

class Pool {
public:
    Pool(int n, bool use_local) : locals_(static_cast<std::size_t>(n)), use_local_(use_local) {
        for (int i = 0; i < n; ++i) {
            workers_.emplace_back([this, i] { worker(i); });
        }
    }

    ~Pool() { shutdown(); }

    void submit(std::function<void()> task) {
        const int idx = tls_index_;
        if (use_local_ && idx >= 0) {
            locals_[static_cast<std::size_t>(idx)].push(std::move(task));
            return;
        }
        {
            std::lock_guard<std::mutex> lock(m_);
            if (done_) {
                return;
            }
            q_.push(std::move(task));
        }
        cv_.notify_one();
    }

    bool run_pending_task() {
        const int idx = tls_index_;
        if (use_local_ && idx >= 0 && !locals_[static_cast<std::size_t>(idx)].empty()) {
            auto task = std::move(locals_[static_cast<std::size_t>(idx)].front());
            locals_[static_cast<std::size_t>(idx)].pop();
            run(task);
            return true;
        }
        std::function<void()> task;
        {
            std::lock_guard<std::mutex> lock(m_);
            if (q_.empty()) {
                return false;
            }
            task = std::move(q_.front());
            q_.pop();
        }
        run(task);
        return true;
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
    static void run(std::function<void()>& task) {
        try {
            task();
        } catch (...) {
        }
    }

    void worker(int i) {
        tls_index_ = i;
        for (;;) {
            std::function<void()> task;
            if (use_local_ && !locals_[static_cast<std::size_t>(i)].empty()) {
                task = std::move(locals_[static_cast<std::size_t>(i)].front());
                locals_[static_cast<std::size_t>(i)].pop();
            } else {
                std::unique_lock<std::mutex> lock(m_);
                cv_.wait(lock, [this] { return done_ || !q_.empty(); });
                if (q_.empty()) {
                    if (done_) {
                        return;
                    }
                    continue;
                }
                task = std::move(q_.front());
                q_.pop();
            }
            run(task);
        }
    }

    static inline thread_local int tls_index_ = -1;

    std::mutex m_;
    std::condition_variable cv_;
    std::queue<std::function<void()>> q_;
    std::vector<std::queue<std::function<void()>>> locals_;
    std::vector<std::thread> workers_;
    bool use_local_;
    bool done_ = false;
};

struct Sample {
    double ms = 0;
    double p99 = 0;
    unsigned long long checksum = 0;
};

Sample one_round(bool use_local) {
    Pool pool(kWorkers, use_local);
    std::barrier gate(kRoots);
    std::latch roots_done(kRoots);
    std::array<std::atomic<int>, kRoots> done;
    for (auto& d : done) {
        d.store(0, std::memory_order_relaxed);
    }
    std::vector<double> latency(static_cast<std::size_t>(kRoots * kChildren));
    std::vector<unsigned long long> values(static_cast<std::size_t>(kRoots * kChildren));

    const auto t0 = Clock::now();
    for (int r = 0; r < kRoots; ++r) {
        pool.submit([&, r] {
            gate.arrive_and_wait();
            for (int i = 0; i < kChildren; ++i) {
                const int idx = r * kChildren + i;
                const auto enqueued = Clock::now();
                pool.submit([&, idx, enqueued] {
                    values[static_cast<std::size_t>(idx)] = work(kIters);
                    latency[static_cast<std::size_t>(idx)] =
                        std::chrono::duration<double, std::milli>(Clock::now() - enqueued).count();
                    done[static_cast<std::size_t>(idx / kChildren)].fetch_add(
                        1, std::memory_order_relaxed);
                });
            }
            while (done[static_cast<std::size_t>(r)].load(std::memory_order_relaxed) < kChildren) {
                if (!pool.run_pending_task()) {
                    std::this_thread::yield();
                }
            }
            roots_done.count_down();
        });
    }
    roots_done.wait();
    const auto t1 = Clock::now();
    pool.shutdown();

    std::sort(latency.begin(), latency.end());
    unsigned long long sum = 0;
    for (auto v : values) {
        sum += v;
    }
    return Sample{std::chrono::duration<double, std::milli>(t1 - t0).count(),
                  latency[latency.size() * 99 / 100], sum};
}

Sample median_of_three(bool use_local) {
    std::array<Sample, kRounds> samples{};
    for (auto& s : samples) {
        s = one_round(use_local);
    }
    std::sort(samples.begin(), samples.end(),
              [](const Sample& a, const Sample& b) { return a.ms < b.ms; });
    return samples[kRounds / 2];
}

}  // namespace

int main() {
    const Sample global = median_of_three(false);
    const Sample local = median_of_three(true);
    std::printf("scheme,ms,p99_ms\n");
    std::printf("global,%.3f,%.3f\n", global.ms, global.p99);
    std::printf("local,%.3f,%.3f\n", local.ms, local.p99);
    std::printf("# checksum %llu %llu\n", global.checksum, local.checksum);
    return global.checksum == local.checksum ? 0 : 1;
}
