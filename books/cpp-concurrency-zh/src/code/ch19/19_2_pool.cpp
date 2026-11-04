// mode: run
// expect: 完成 1000，校验和 499500
// 4 个工作线程、容量 4 的有界队列；shutdown 后校验全部任务都完成。
#include <atomic>
#include <condition_variable>
#include <cstddef>
#include <cstdio>
#include <functional>
#include <mutex>
#include <queue>
#include <thread>
#include <utility>
#include <vector>

namespace {

class Pool {
public:
    Pool(int workers, std::size_t capacity) : capacity_(capacity) {
        for (int i = 0; i < workers; ++i) {
            workers_.emplace_back([this] { worker(); });
        }
    }

    ~Pool() { shutdown(); }

    bool submit(std::function<void()> task) {
        std::unique_lock<std::mutex> lock(m_);
        not_full_.wait(lock, [this] { return done_ || q_.size() < capacity_; });
        if (done_) {
            return false;  // 关闭后不再接受新任务
        }
        q_.push(std::move(task));
        not_empty_.notify_one();
        return true;
    }

    void shutdown() {
        {
            std::lock_guard<std::mutex> lock(m_);
            if (done_) {
                return;
            }
            done_ = true;
        }
        not_empty_.notify_all();  // 只置标志不唤醒，工作线程会一直睡在 wait 上
        not_full_.notify_all();
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
                not_empty_.wait(lock, [this] { return done_ || !q_.empty(); });
                if (q_.empty()) {
                    return;  // done 且已经取空
                }
                task = std::move(q_.front());
                q_.pop();
            }
            not_full_.notify_one();  // 让阻塞的 submit 继续
            try {
                task();
            } catch (...) {
                failed_.fetch_add(1, std::memory_order_relaxed);
            }
        }
    }

    std::mutex m_;
    std::condition_variable not_empty_;
    std::condition_variable not_full_;
    std::queue<std::function<void()>> q_;
    std::vector<std::thread> workers_;
    std::size_t capacity_;
    bool done_ = false;
    std::atomic<int> failed_{0};
};

}  // namespace

int main() {
    constexpr int kTasks = 1000;
    std::atomic<int> completed{0};
    std::atomic<long long> sum{0};

    {
        Pool pool(4, 4);
        for (int i = 0; i < kTasks; ++i) {
            if (!pool.submit([&, i] {
                    sum.fetch_add(i, std::memory_order_relaxed);
                    completed.fetch_add(1, std::memory_order_relaxed);
                })) {
                std::printf("提交被拒绝：%d\n", i);
            }
        }
    }

    std::printf("完成 %d，校验和 %lld\n", completed.load(), sum.load());
    return completed.load() == kTasks ? 0 : 1;
}
