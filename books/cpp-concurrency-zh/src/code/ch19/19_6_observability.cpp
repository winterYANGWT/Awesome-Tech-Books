// mode: run
// expect: 线程名: main pool-0 pool-1 pool-2 pool-3\n队列长度: 96\n活跃线程: 4\n已完成: 100
// 给工作线程命名，读 /proc/self/task/*/comm，并打印队列和活跃线程数。
#ifndef _GNU_SOURCE
#define _GNU_SOURCE  // pthread_setname_np 需要
#endif
#include <algorithm>
#include <atomic>
#include <condition_variable>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <functional>
#include <latch>
#include <mutex>
#include <pthread.h>
#include <queue>
#include <string>
#include <thread>
#include <utility>
#include <vector>

namespace {

class Pool {
public:
    explicit Pool(int n) {
        for (int i = 0; i < n; ++i) {
            workers_.emplace_back([this, i] {
                const std::string name = "pool-" + std::to_string(i);
                pthread_setname_np(pthread_self(), name.c_str());
                worker();
            });
        }
    }

    ~Pool() { shutdown(); }

    bool submit(std::function<void()> task) {
        {
            std::lock_guard<std::mutex> lock(m_);
            if (done_) {
                return false;
            }
            q_.push(std::move(task));
        }
        cv_.notify_one();
        return true;
    }

    std::size_t queue_size() const {
        std::lock_guard<std::mutex> lock(m_);
        return q_.size();
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
            try {
                task();
            } catch (...) {
            }
        }
    }

    mutable std::mutex m_;
    std::condition_variable cv_;
    std::queue<std::function<void()>> q_;
    std::vector<std::thread> workers_;
    bool done_ = false;
};

std::vector<std::string> thread_names() {
    std::vector<std::string> names;
    for (const auto& entry : std::filesystem::directory_iterator("/proc/self/task")) {
        std::ifstream in(entry.path() / "comm");
        std::string name;
        if (std::getline(in, name)) {
            names.push_back(name);
        }
    }
    std::sort(names.begin(), names.end());
    return names;
}

}  // namespace

int main() {
    pthread_setname_np(pthread_self(), "main");
    Pool pool(4);
    std::latch gate(1);
    std::mutex m;
    std::condition_variable cv;
    int active = 0;
    std::atomic<int> finished{0};

    for (int i = 0; i < 100; ++i) {
        pool.submit([&] {
            {
                std::lock_guard<std::mutex> lock(m);
                ++active;
            }
            cv.notify_all();
            gate.wait();
            {
                std::lock_guard<std::mutex> lock(m);
                --active;
            }
            finished.fetch_add(1, std::memory_order_relaxed);
        });
    }

    {
        std::unique_lock<std::mutex> lock(m);
        cv.wait(lock, [&] { return active == 4; });
    }

    const std::vector<std::string> names = thread_names();
    std::printf("线程名:");
    for (const auto& name : names) {
        std::printf(" %s", name.c_str());
    }
    std::printf("\n队列长度: %zu\n", pool.queue_size());
    std::printf("活跃线程: %d\n", active);

    gate.count_down();
    pool.shutdown();
    std::printf("已完成: %d\n", finished.load());
    return 0;
}
