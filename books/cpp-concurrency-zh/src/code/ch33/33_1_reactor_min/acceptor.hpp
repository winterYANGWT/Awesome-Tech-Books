#pragma once

#include "connection.hpp"
#include "event_loop.hpp"

// 监听器也是一个 Handler：它只做 accept4，然后把连接交给 ConnectionManager。
class Acceptor : public Handler {
public:
    Acceptor(EventLoop &loop, int listen_fd, ConnectionManager &manager)
        : loop_(loop), fd_(listen_fd), manager_(manager) {}

    int fd() const override { return fd_; }
    void handle_event(std::uint32_t events) override;

private:
    EventLoop &loop_;
    int fd_;
    ConnectionManager &manager_;
};
