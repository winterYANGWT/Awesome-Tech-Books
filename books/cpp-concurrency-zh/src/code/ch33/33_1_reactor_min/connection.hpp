#pragma once

#include "event_loop.hpp"

#include <memory>
#include <string>
#include <unordered_map>

class ConnectionManager;

// 一条回射连接：读到的字节原样写回；3 秒没有读写就关掉。
class Connection : public Handler, public std::enable_shared_from_this<Connection> {
public:
    Connection(EventLoop &loop, int fd, ConnectionManager &manager);
    ~Connection() override;

    int fd() const override { return fd_; }
    void handle_event(std::uint32_t events) override;
    void start();

private:
    void on_read();
    void on_write();
    void reset_idle();
    void close(const char *reason);

    EventLoop &loop_;
    ConnectionManager &manager_;
    int fd_;
    std::string out_;
    std::uint64_t timer_id_ = 0;
    bool closed_ = false;
};

class ConnectionManager {
public:
    explicit ConnectionManager(EventLoop &loop) : loop_(loop) {}
    void accept_all(int listen_fd);
    void remove(int fd);

private:
    EventLoop &loop_;
    std::unordered_map<int, std::shared_ptr<Connection>> conns_;
};
