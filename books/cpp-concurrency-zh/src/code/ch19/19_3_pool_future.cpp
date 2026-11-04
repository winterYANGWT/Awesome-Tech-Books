// mode: run
// expect: 任务 42 抛出：任务 42 失败\n收到 99 个结果，和 326586
// submit 返回 future<int>；packaged_task 只能移动，所以用 shared_ptr 包一层。
#include <condition_variable>
#include <cstdio>
#include <functional>
#include <future>
#include <memory>
#include <mutex>
#include <queue>
#include <stdexcept>
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
            if (done_) {
                throw std::runtime_error("池已关闭");
            }
            q_.push([task] { (*task)(); });  // packaged_task 把值或异常存进共享状态
        }
        cv_.notify_one();
        return result;
    }

    void shutdown() {
        {
            std::lock_guard<std::mutex> lock(m_);
            if (done_) {
                return;
            }
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
            task();  // packaged_task 会接住任务异常，不会让它逃出工作线程
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
    Pool pool(4);
    std::vector<std::future<int>> futures;
    futures.reserve(100);

    for (int i = 0; i < 100; ++i) {
        futures.push_back(pool.submit([i] {
            if (i == 42) {
                throw std::runtime_error("任务 42 失败");
            }
            return i * i;
        }));
    }

    long long sum = 0;
    int received = 0;
    for (int i = 0; i < 100; ++i) {
        try {
            sum += futures[static_cast<std::size_t>(i)].get();
            ++received;
        } catch (const std::exception& e) {
            std::printf("任务 %d 抛出：%s\n", i, e.what());
        }
    }
    std::printf("收到 %d 个结果，和 %lld\n", received, sum);
    return 0;
}
