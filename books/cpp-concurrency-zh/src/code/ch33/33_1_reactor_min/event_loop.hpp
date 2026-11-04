#pragma once

#include "timer_queue.hpp"

#include <cstdint>
#include <memory>
#include <vector>

// 事件处理器：连接、监听器、定时器都实现它；Reactor 只认 fd 和回调。
class Handler {
public:
    virtual ~Handler() = default;
    virtual int fd() const = 0;
    virtual void handle_event(std::uint32_t events) = 0;
};

class EventLoop {
public:
    EventLoop();
    ~EventLoop();
    EventLoop(const EventLoop &) = delete;
    EventLoop &operator=(const EventLoop &) = delete;

    void add(Handler *h, std::uint32_t events);
    void mod(Handler *h, std::uint32_t events);
    void remove(Handler *h);
    // 回调里关闭自己时，把 shared_ptr 放进这里，等这一批事件处理完再析构。
    void retire(std::shared_ptr<Handler> h);
    void run();
    void stop() { stop_ = true; }
    TimerQueue &timers() { return timers_; }

private:
    int ep_ = -1;
    bool stop_ = false;
    TimerQueue timers_;
    std::vector<std::shared_ptr<Handler>> retired_;
};
