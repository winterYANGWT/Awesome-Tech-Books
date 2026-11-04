// mode: run
// expect: 校验和 1212（8 个子任务全部完成）
// 同样 4 个父任务等子任务，但等待时先执行队列里的任务。
#include <atomic>
#include <chrono>
#include <condition_variable>
#include <cstdio>
#include <functional>
#include <future>
#include <latch>
#include <memory>
#include <mutex>
#include <queue>
#include <thread>
#include <type_traits>
#include <utility>
#include <vector>

namespace {

class Pool {
public:
    explicit Pool(int n) {
        for (int i = 0; i < n; ++i) {
            workers_.emplace_back([this] { worker(); });
        }
    }

    ~Pool() { shutdown(); }

    template <class F>
    auto submit(F&& f) -> std::future<std::invoke_result_t<F>> {
        using R = std::invoke_result_t<F>;
        auto task = std::make_shared<std::packaged_task<R()>>(std::forward<F>(f));
        std::future<R> result = task->get_future();
        {
            std::lock_guard<std::mutex> lock(m_);
            q_.push([task] { (*task)(); });
        }
        cv_.notify_one();
        return result;
    }

    // 等待者帮忙：取不到任务就返回 false，由调用者决定 yield 还是继续等。
    bool run_pending_task() {
        std::function<void()> task;
        {
            std::lock_guard<std::mutex> lock(m_);
            if (q_.empty()) {
                return false;
            }
            task = std::move(q_.front());
            q_.pop();
        }
        try {
            task();
        } catch (...) {
            // 任务自己的异常由 packaged_task 存进 future，这里只兜底
        }
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
            task();
        }
    }

    std::mutex m_;
    std::condition_variable cv_;
    std::queue<std::function<void()>> q_;
    std::vector<std::thread> workers_;
    bool done_ = false;
};

}  // namespace

int main() {
    std::latch started(4);
    std::latch gate(4);
    std::latch roots_done(4);
    std::atomic<long long> checksum{0};
    Pool pool(4);

    for (int p = 0; p < 4; ++p) {
        pool.submit([&, p] {
            started.count_down();
            gate.arrive_and_wait();
            auto a = pool.submit([p] { return p * 100 + 1; });
            auto b = pool.submit([p] { return p * 100 + 2; });

            auto wait_with_help = [&pool](std::future<int>& f) {
                while (f.wait_for(std::chrono::milliseconds(0)) !=
                       std::future_status::ready) {
                    if (!pool.run_pending_task()) {
                        std::this_thread::yield();
                    }
                }
            };
            wait_with_help(a);
            wait_with_help(b);
            checksum.fetch_add(a.get() + b.get(), std::memory_order_relaxed);
            roots_done.count_down();
        });
    }

    started.wait();
    roots_done.wait();
    pool.shutdown();
    std::printf("校验和 %lld（8 个子任务全部完成）\n", checksum.load());
    return checksum.load() == 1212 ? 0 : 1;
}
