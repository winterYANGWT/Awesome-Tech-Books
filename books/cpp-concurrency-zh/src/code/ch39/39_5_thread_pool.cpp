// mode: run
// 最小线程池：任务队列 + 条件变量 + close；空队列时 worker 睡眠，不用 yield 自旋。
#include <condition_variable>
#include <functional>
#include <iostream>
#include <mutex>
#include <queue>
#include <stdexcept>
#include <thread>
#include <vector>

class ThreadPool {
public:
    explicit ThreadPool(unsigned n) {
        for (unsigned i = 0; i < n; ++i) {
            workers_.emplace_back([this] { worker(); });
        }
    }

    ~ThreadPool() { close(); }

    void submit(std::function<void()> job) {
        {
            std::lock_guard<std::mutex> lock(mutex_);
            if (done_) {
                throw std::runtime_error("submit after close");
            }
            jobs_.push(std::move(job));
        }
        cv_.notify_one();
    }

    void close() {
        {
            std::lock_guard<std::mutex> lock(mutex_);
            if (done_) {
                return;
            }
            done_ = true;
        }
        cv_.notify_all();
        for (auto& t : workers_) {
            t.join();
        }
        workers_.clear();
    }

private:
    void worker() {
        for (;;) {
            std::function<void()> job;
            {
                std::unique_lock<std::mutex> lock(mutex_);
                cv_.wait(lock, [this] { return done_ || !jobs_.empty(); });
                if (done_ && jobs_.empty()) {
                    return;
                }
                job = std::move(jobs_.front());
                jobs_.pop();
            }
            job();
        }
    }

    std::mutex mutex_;
    std::condition_variable cv_;
    std::queue<std::function<void()>> jobs_;
    bool done_ = false;
    std::vector<std::thread> workers_;
};

int main() {
    constexpr int kTasks = 8;
    std::vector<int> results(kTasks, 0);
    ThreadPool pool(4);

    for (int i = 0; i < kTasks; ++i) {
        pool.submit([i, &results] { results[i] = i * i; });
    }
    pool.close();

    int total = 0;
    for (int i = 0; i < kTasks; ++i) {
        std::cout << "task " << i << " -> " << results[i] << '\n';
        total += results[i];
    }
    std::cout << "total=" << total << '\n';
}
