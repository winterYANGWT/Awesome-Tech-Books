#include "event_loop.hpp"

#include <sys/eventfd.h>
#include <unistd.h>

#include <algorithm>
#include <cerrno>
#include <system_error>

namespace {
constexpr std::size_t kMaxPending = 4096;
}

EventLoop::EventLoop()
    : ep_(::epoll_create1(EPOLL_CLOEXEC)),
      efd_(::eventfd(0, EFD_NONBLOCK | EFD_CLOEXEC)) {
    if (ep_ < 0)
        throw std::system_error(errno, std::generic_category(), "epoll_create1");
    if (efd_ < 0) {
        ::close(ep_);
        throw std::system_error(errno, std::generic_category(), "eventfd");
    }
    add(this, EPOLLIN);  // eventfd 自己也是一个 Handler
}

EventLoop::~EventLoop() {
    retired_.clear();
    if (efd_ >= 0) ::close(efd_);
    if (ep_ >= 0) ::close(ep_);
}

void EventLoop::add(Handler *h, std::uint32_t events) {
    epoll_event ev{};
    ev.events = events;
    ev.data.ptr = h;
    if (::epoll_ctl(ep_, EPOLL_CTL_ADD, h->fd(), &ev) != 0)
        throw std::system_error(errno, std::generic_category(), "epoll_ctl(ADD)");
}

void EventLoop::mod(Handler *h, std::uint32_t events) {
    epoll_event ev{};
    ev.events = events;
    ev.data.ptr = h;
    if (::epoll_ctl(ep_, EPOLL_CTL_MOD, h->fd(), &ev) != 0)
        throw std::system_error(errno, std::generic_category(), "epoll_ctl(MOD)");
}

void EventLoop::remove(Handler *h) { ::epoll_ctl(ep_, EPOLL_CTL_DEL, h->fd(), nullptr); }

void EventLoop::retire(std::shared_ptr<Handler> h) { retired_.push_back(std::move(h)); }

// ANCHOR: wake
void EventLoop::wakeup() {
    const std::uint64_t one = 1;
    const ssize_t n = ::write(efd_, &one, sizeof one);
    (void)n;  // 计数器溢出前不会失败；EAGAIN 说明循环已经醒着
}

bool EventLoop::run_in_loop(std::function<void()> task) {
    if (std::this_thread::get_id() == loop_thread_) {
        task();
        return true;
    }
    {
        std::lock_guard<std::mutex> lock(mu_);
        if (pending_.size() >= kMaxPending) return false;
        pending_.push_back(std::move(task));
    }
    wakeup();
    return true;
}
// ANCHOR_END: wake

void EventLoop::quit() {
    quit_ = true;
    wakeup();
}

void EventLoop::handle_event(std::uint32_t) {
    std::uint64_t count = 0;
    while (::read(efd_, &count, sizeof count) > 0) {
    }
}

std::uint64_t EventLoop::add_timer(Clock::time_point when, std::function<void()> cb) {
    const std::uint64_t id = next_timer_++;
    timers_.push(Timer{when, id, std::move(cb)});
    return id;
}

void EventLoop::cancel_timer(std::uint64_t id) {
    if (id != 0) cancelled_.insert(id);
}

int EventLoop::timeout_ms(Clock::time_point now) const {
    if (timers_.empty()) return -1;
    const auto left = timers_.top().when - now;
    if (left <= Clock::duration::zero()) return 0;
    const auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(left);
    return static_cast<int>(std::max<std::int64_t>(1, ms.count()));
}

void EventLoop::run_expired(Clock::time_point now) {
    while (!timers_.empty() && timers_.top().when <= now) {
        Timer timer = timers_.top();
        timers_.pop();
        if (cancelled_.erase(timer.id) != 0) continue;
        timer.cb();
    }
}

// ANCHOR: loop
void EventLoop::run() {
    loop_thread_ = std::this_thread::get_id();
    epoll_event events[64];
    while (!quit_) {
        const int n = ::epoll_wait(ep_, events, 64, timeout_ms(Clock::now()));
        if (n < 0) {
            if (errno == EINTR) continue;
            throw std::system_error(errno, std::generic_category(), "epoll_wait");
        }
        for (int i = 0; i < n; ++i) {
            auto *h = static_cast<Handler *>(events[i].data.ptr);
            h->handle_event(events[i].events);
        }
        run_expired(Clock::now());

        // 先换出队列再执行：回调里再投递任务不会死锁，也不会插队。
        std::vector<std::function<void()>> ready;
        {
            std::lock_guard<std::mutex> lock(mu_);
            ready.swap(pending_);
        }
        for (auto &task : ready) task();
        retired_.clear();
    }
}
// ANCHOR_END: loop
