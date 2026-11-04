// mode: run
// expect: 排空：完成 100，丢弃 0\n丢弃：完成 [0-9]+，丢弃 [1-9][0-9]*，合计 100
// 两种关闭语义：drain 排空队列，discard 立即停并把未执行的任务计数。
#include <atomic>
#include <chrono>
#include <condition_variable>
#include <cstdio>
#include <functional>
#include <latch>
#include <mutex>
#include <queue>
#include <thread>
#include <utility>
#include <vector>

namespace {

enum class Shutdown { drain, discard };

struct Stats {
    int completed = 0;
    int dropped = 0;
};

class Pool {
public:
    explicit Pool(int n) {
        for (int i = 0; i < n; ++i) {
            workers_.emplace_back([this] { worker(); });
        }
    }

    ~Pool() { shutdown(Shutdown::drain); }

    bool submit(std::function<void()> task) {
        {
            std::lock_guard<std::mutex> lock(m_);
            if (stopping_) {
                return false;
            }
            q_.push(std::move(task));
        }
        cv_.notify_one();
        return true;
    }

    Stats shutdown(Shutdown mode) {
        {
            std::lock_guard<std::mutex> lock(m_);
            stopping_ = true;
            discard_ = (mode == Shutdown::discard);
        }
        cv_.notify_all();
        for (auto& t : workers_) {
            if (t.joinable()) {
                t.join();
            }
        }
        std::lock_guard<std::mutex> lock(m_);
        return Stats{completed_, dropped_};
    }

private:
    void worker() {
        for (;;) {
            std::function<void()> task;
            {
                std::unique_lock<std::mutex> lock(m_);
                cv_.wait(lock, [this] { return stopping_ || !q_.empty(); });
                if (discard_) {
                    dropped_ += static_cast<int>(q_.size());
                    q_ = {};
                    return;
                }
                if (q_.empty()) {
                    if (stopping_) {
                        return;
                    }
                    continue;
                }
                task = std::move(q_.front());
                q_.pop();
            }
            task();
            {
                std::lock_guard<std::mutex> lock(m_);
                ++completed_;
            }
        }
    }

    std::mutex m_;
    std::condition_variable cv_;
    std::queue<std::function<void()>> q_;
    std::vector<std::thread> workers_;
    bool stopping_ = false;
    bool discard_ = false;
    int completed_ = 0;
    int dropped_ = 0;
};

}  // namespace

int main() {
    {
        Pool pool(2);
        for (int i = 0; i < 100; ++i) {
            pool.submit([] { std::this_thread::sleep_for(std::chrono::milliseconds(2)); });
        }
        const Stats s = pool.shutdown(Shutdown::drain);
        std::printf("排空：完成 %d，丢弃 %d\n", s.completed, s.dropped);
    }

    {
        Pool pool(2);
        std::latch started(2);
        for (int i = 0; i < 100; ++i) {
            pool.submit([&started, i] {
                if (i < 2) {
                    started.count_down();
                }
                std::this_thread::sleep_for(std::chrono::milliseconds(5));
            });
        }
        started.wait();
        const Stats s = pool.shutdown(Shutdown::discard);
        std::printf("丢弃：完成 %d，丢弃 %d，合计 %d\n", s.completed, s.dropped,
                    s.completed + s.dropped);
    }
    return 0;
}
