#include "event_loop.hpp"

#include <sys/epoll.h>
#include <unistd.h>

#include <cerrno>
#include <system_error>

EventLoop::EventLoop() : ep_(::epoll_create1(EPOLL_CLOEXEC)) {
    if (ep_ < 0)
        throw std::system_error(errno, std::generic_category(), "epoll_create1");
}

EventLoop::~EventLoop() {
    retired_.clear();
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

void EventLoop::remove(Handler *h) {
    ::epoll_ctl(ep_, EPOLL_CTL_DEL, h->fd(), nullptr);
}

void EventLoop::retire(std::shared_ptr<Handler> h) { retired_.push_back(std::move(h)); }

void EventLoop::run() {
    epoll_event events[64];
    while (!stop_) {
        const auto now = TimerQueue::Clock::now();
        const int n = ::epoll_wait(ep_, events, 64, timers_.timeout_ms(now));
        if (n < 0) {
            if (errno == EINTR) continue;
            throw std::system_error(errno, std::generic_category(), "epoll_wait");
        }
        for (int i = 0; i < n; ++i) {
            auto *h = static_cast<Handler *>(events[i].data.ptr);
            h->handle_event(events[i].events);
        }
        timers_.run_expired(TimerQueue::Clock::now());
        retired_.clear();  // 这一批事件里 retire 的对象到这里才析构
    }
}
