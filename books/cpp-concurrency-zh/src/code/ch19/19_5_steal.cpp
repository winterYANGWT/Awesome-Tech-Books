// mode: run
// output: varies
// tsan: no（计时示例，TSan 会改变窃取路径的开销）
// expect: scheme,ms,p99_ms\nlocal,\d+\.\d+,\d+\.\d+\nsteal,\d+\.\d+,\d+\.\d+\n# checksum \d+
// 任务长度不均匀：本地队列不偷 vs 空闲线程从队尾偷。
#include <algorithm>
#include <array>
#include <atomic>
#include <barrier>
#include <chrono>
#include <condition_variable>
#include <cstdint>
#include <cstdio>
#include <deque>
#include <functional>
#include <latch>
#include <mutex>
#include <thread>
#include <utility>
#include <vector>

namespace {

using Clock = std::chrono::steady_clock;

constexpr int kWorkers = 4;
constexpr int kRoots = 4;
constexpr int kChildren = 1200;
constexpr int kIters = 3000;
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
    Pool(int n, bool steal) : n_(n), steal_(steal), locals_(static_cast<std::size_t>(n)) {
        for (int i = 0; i < n; ++i) {
            workers_.emplace_back([this, i] { worker(i); });
        }
    }

    ~Pool() { shutdown(); }

    void submit(std::function<void()> task) {
        const int idx = tls_index_;
        if (idx >= 0) {
            Local& local = locals_[static_cast<std::size_t>(idx)];
            {
                std::lock_guard<std::mutex> lock(local.m);
                local.q.push_front(std::move(task));  // 新任务在前：后进先出
            }
            pending_.fetch_add(1, std::memory_order_relaxed);
            cv_.notify_one();
            return;
        }
        {
            std::lock_guard<std::mutex> lock(m_);
            global_.push_back(std::move(task));
        }
        cv_.notify_one();
    }

    bool run_pending_task() {
        const int idx = tls_index_;
        std::function<void()> task;
        if (idx >= 0 && try_pop_local(idx, task)) {
            run(task);
            return true;
        }
        if (idx >= 0 && try_steal(idx, task)) {
            run(task);
            return true;
        }
        {
            std::lock_guard<std::mutex> lock(m_);
            if (global_.empty()) {
                return false;
            }
            task = std::move(global_.front());
            global_.pop_front();
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
    struct Local {
        std::deque<std::function<void()>> q;
        std::mutex m;
    };

    static void run(std::function<void()>& task) {
        try {
            task();
        } catch (...) {
        }
    }

    bool try_pop_local(int i, std::function<void()>& out) {
        Local& local = locals_[static_cast<std::size_t>(i)];
        std::lock_guard<std::mutex> lock(local.m);
        if (local.q.empty()) {
            return false;
        }
        out = std::move(local.q.front());
        local.q.pop_front();
        pending_.fetch_sub(1, std::memory_order_relaxed);
        return true;
    }

    bool try_steal(int i, std::function<void()>& out) {
        if (!steal_) {
            return false;
        }
        for (int k = 1; k < n_; ++k) {
            const int j = (i + k) % n_;
            Local& local = locals_[static_cast<std::size_t>(j)];
            std::lock_guard<std::mutex> lock(local.m);
            if (local.q.empty()) {
                continue;
            }
            out = std::move(local.q.back());  // 从队尾偷：和 owner 的前端错开
            local.q.pop_back();
            pending_.fetch_sub(1, std::memory_order_relaxed);
            return true;
        }
        return false;
    }

    void worker(int i) {
        tls_index_ = i;
        for (;;) {
            std::function<void()> task;
            if (try_pop_local(i, task)) {
                run(task);
                continue;
            }
            if (try_steal(i, task)) {
                run(task);
                continue;
            }
            {
                std::unique_lock<std::mutex> lock(m_);
                cv_.wait(lock, [this] {
                    return done_ || !global_.empty() ||
                           (steal_ && pending_.load(std::memory_order_relaxed) > 0);
                });
                if (!global_.empty()) {
                    task = std::move(global_.front());
                    global_.pop_front();
                } else if (done_ && pending_.load(std::memory_order_relaxed) == 0) {
                    return;
                } else {
                    continue;  // 被本地队列的通知叫醒，回去再偷一次
                }
            }
            run(task);
        }
    }

    static inline thread_local int tls_index_ = -1;

    int n_;
    bool steal_;
    std::mutex m_;
    std::condition_variable cv_;
    std::deque<std::function<void()>> global_;
    std::vector<Local> locals_;
    std::vector<std::thread> workers_;
    std::atomic<int> pending_{0};
    bool done_ = false;
};

struct Sample {
    double ms = 0;
    double p99 = 0;
    unsigned long long checksum = 0;
};

Sample one_round(bool steal) {
    Pool pool(kWorkers, steal);
    std::barrier gate(kRoots);
    std::latch roots_done(kRoots);
    std::array<std::atomic<int>, kRoots> done;
    for (auto& d : done) {
        d.store(0, std::memory_order_relaxed);
    }
    const std::array<int, kRoots> cost{1, 2, 4, 8};
    std::vector<double> latency(static_cast<std::size_t>(kRoots * kChildren));
    std::vector<unsigned long long> values(static_cast<std::size_t>(kRoots * kChildren));

    const auto t0 = Clock::now();
    for (int r = 0; r < kRoots; ++r) {
        pool.submit([&, r] {
            gate.arrive_and_wait();
            for (int i = 0; i < kChildren; ++i) {
                const int idx = r * kChildren + i;
                const auto enqueued = Clock::now();
                pool.submit([&, idx, enqueued, r] {
                    values[static_cast<std::size_t>(idx)] = work(kIters * cost[r]);
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

Sample median_of_three(bool steal) {
    std::array<Sample, kRounds> samples{};
    for (auto& s : samples) {
        s = one_round(steal);
    }
    std::sort(samples.begin(), samples.end(),
              [](const Sample& a, const Sample& b) { return a.ms < b.ms; });
    return samples[kRounds / 2];
}

}  // namespace

int main() {
    const Sample local = median_of_three(false);
    const Sample steal = median_of_three(true);
    std::printf("scheme,ms,p99_ms\n");
    std::printf("local,%.3f,%.3f\n", local.ms, local.p99);
    std::printf("steal,%.3f,%.3f\n", steal.ms, steal.p99);
    std::printf("# checksum %llu %llu\n", local.checksum, steal.checksum);
    return local.checksum == steal.checksum ? 0 : 1;
}
