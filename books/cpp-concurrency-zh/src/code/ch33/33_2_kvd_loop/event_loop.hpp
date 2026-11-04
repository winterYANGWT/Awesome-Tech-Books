#pragma once

#include <sys/epoll.h>

#include <chrono>
#include <cstdint>
#include <functional>
#include <memory>
#include <mutex>
#include <queue>
#include <thread>
#include <unordered_set>
#include <vector>

class Handler {
public:
    virtual ~Handler() = default;
    virtual int fd() const = 0;
    virtual void handle_event(std::uint32_t events) = 0;
};

// 单线程事件循环：epoll + eventfd 唤醒 + 最小堆定时器 + 待执行任务。
class EventLoop : public Handler {
public:
    EventLoop();
    ~EventLoop() override;

    int fd() const override { return efd_; }
    void handle_event(std::uint32_t events) override;

    void add(Handler *h, std::uint32_t events);
    void mod(Handler *h, std::uint32_t events);
    void remove(Handler *h);
    void retire(std::shared_ptr<Handler> h);

    void run();
    void quit();
    bool run_in_loop(std::function<void()> task);

    std::uint64_t add_timer(std::chrono::steady_clock::time_point when,
                            std::function<void()> cb);
    void cancel_timer(std::uint64_t id);

private:
    using Clock = std::chrono::steady_clock;
    struct Timer {
        Clock::time_point when;
        std::uint64_t id;
        std::function<void()> cb;
    };
    struct Later {
        bool operator()(const Timer &a, const Timer &b) const { return a.when > b.when; }
    };

    int timeout_ms(Clock::time_point now) const;
    void run_expired(Clock::time_point now);
    void wakeup();

    int ep_ = -1;
    int efd_ = -1;
    bool quit_ = false;
    std::thread::id loop_thread_;
    std::mutex mu_;
    std::vector<std::function<void()>> pending_;
    std::priority_queue<Timer, std::vector<Timer>, Later> timers_;
    std::unordered_set<std::uint64_t> cancelled_;
    std::uint64_t next_timer_ = 1;
    std::vector<std::shared_ptr<Handler>> retired_;
};
