// mode: hang
// expect: 仍未结束
// timeout: 2
// 反例：4 个父任务各提交两个子任务，然后等 future；4 个工作线程全在等。
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
    std::atomic<long long> checksum{0};
    Pool pool(4);

    for (int p = 0; p < 4; ++p) {
        pool.submit([&, p] {
            started.count_down();
            gate.arrive_and_wait();  // 让 4 个父任务同时进入等待
            auto a = pool.submit([p] { return p * 100 + 1; });
            auto b = pool.submit([p] { return p * 100 + 2; });
            checksum.fetch_add(a.get() + b.get(), std::memory_order_relaxed);
        });
    }

    started.wait();
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    std::printf("4 个父任务都已开始，8 个子任务还在队列里\n");
    // 主线程保持存活，2 秒后 gdb 能同时看到它和 4 个工作线程。
    std::this_thread::sleep_for(std::chrono::seconds(30));
    return 0;
}
