// mode: run
// expect: 处理 2 个，异常 1 个，剩余 0 个
// 队列存 shared_ptr，wait_pop 只拷贝指针（不抛）；任务执行抛异常时捕获并继续。
// 异常路径再补一次 notify_all，让其他消费者重新检查队列。
#include <atomic>
#include <condition_variable>
#include <cstdio>
#include <memory>
#include <mutex>
#include <queue>
#include <stdexcept>
#include <thread>
#include <utility>
#include <vector>

namespace {

struct Job {
    int id;
    bool fail;

    void run() const {
        if (fail) {
            throw std::runtime_error("任务执行失败");
        }
    }
};

class JobQueue {
public:
    bool push(std::shared_ptr<Job> value) {
        {
            std::lock_guard<std::mutex> lock(m_);
            if (closed_) {
                return false;
            }
            q_.push(std::move(value));
        }
        cv_.notify_one();
        return true;
    }

    std::shared_ptr<Job> wait_pop() {
        std::unique_lock<std::mutex> lock(m_);
        cv_.wait(lock, [&] { return !q_.empty() || closed_; });
        if (q_.empty()) {
            return nullptr;
        }
        auto item = q_.front();
        q_.pop();
        return item;
    }

    void notify_all() { cv_.notify_all(); }

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

    std::size_t size() const {
        std::lock_guard<std::mutex> lock(m_);
        return q_.size();
    }

private:
    mutable std::mutex m_;
    std::condition_variable cv_;
    std::queue<std::shared_ptr<Job>> q_;
    bool closed_ = false;
};

}  // namespace

int main() {
    JobQueue q;
    std::atomic<int> processed{0};
    std::atomic<int> exceptions{0};

// ANCHOR: worker
    auto worker = [&] {
        for (;;) {
            auto job = q.wait_pop();
            if (!job) {
                return;
            }
            try {
                job->run();
                processed.fetch_add(1, std::memory_order_relaxed);
            } catch (const std::exception& e) {
                exceptions.fetch_add(1, std::memory_order_relaxed);
                std::printf("任务 %d 抛异常：%s\n", job->id, e.what());
                q.notify_all();  // 异常路径补一次通知
            }
        }
    };
// ANCHOR_END: worker

    std::thread a(worker);
    std::thread b(worker);
    q.push(std::make_shared<Job>(Job{1, false}));
    q.push(std::make_shared<Job>(Job{2, true}));
    q.push(std::make_shared<Job>(Job{3, false}));
    q.close();

    a.join();
    b.join();
    std::printf("处理 %d 个，异常 %d 个，剩余 %zu 个\n", processed.load(),
                exceptions.load(), q.size());
    return 0;
}
