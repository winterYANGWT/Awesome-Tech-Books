#include "connection.hpp"

#include <sys/epoll.h>
#include <sys/socket.h>
#include <unistd.h>

#include <cerrno>
#include <cstdio>
#include <system_error>

Connection::Connection(EventLoop &loop, int fd, ConnectionManager &manager)
    : loop_(loop), manager_(manager), fd_(fd) {}

Connection::~Connection() {
    if (fd_ >= 0) ::close(fd_);
}

void Connection::start() {
    loop_.add(this, EPOLLIN);
    reset_idle();
    std::printf("accepted fd=%d\n", fd_);
}

void Connection::reset_idle() {
    loop_.timers().cancel(timer_id_);
    std::weak_ptr<Connection> weak = weak_from_this();
    timer_id_ = loop_.timers().add(TimerQueue::Clock::now() + std::chrono::seconds(3),
                                   [weak] {
                                       if (auto self = weak.lock()) {
                                           std::printf("idle timeout fd=%d\n", self->fd_);
                                           self->timer_id_ = 0;
                                           self->close("idle timeout");
                                       }
                                   });
}

void Connection::handle_event(std::uint32_t events) {
    if (events & (EPOLLERR | EPOLLHUP)) {
        close("error/hup");
        return;
    }
    if (events & EPOLLIN) on_read();
    if (!closed_ && (events & EPOLLOUT)) on_write();
}

void Connection::on_read() {
    char buf[4096];
    for (;;) {
        const ssize_t n = ::recv(fd_, buf, sizeof buf, 0);
        if (n > 0) {
            out_.append(buf, static_cast<std::size_t>(n));
            std::printf("echo fd=%d bytes=%zd\n", fd_, n);
            reset_idle();
            continue;
        }
        if (n == 0) {
            close("peer closed");
            return;
        }
        if (errno == EINTR) continue;
        if (errno == EAGAIN || errno == EWOULDBLOCK) break;
        close("recv error");
        return;
    }
    on_write();  // 先把能写的写掉，写不完再注册 EPOLLOUT
}

void Connection::on_write() {
    while (!out_.empty()) {
        const ssize_t n = ::send(fd_, out_.data(), out_.size(), MSG_NOSIGNAL);
        if (n > 0) {
            out_.erase(0, static_cast<std::size_t>(n));
            reset_idle();
            continue;
        }
        if (n < 0 && errno == EINTR) continue;
        if (n < 0 && (errno == EAGAIN || errno == EWOULDBLOCK)) break;
        close("send error");
        return;
    }
    if (out_.empty()) loop_.mod(this, EPOLLIN);
    else loop_.mod(this, EPOLLIN | EPOLLOUT);
}

void Connection::close(const char *reason) {
    if (closed_) return;
    closed_ = true;
    loop_.remove(this);
    loop_.timers().cancel(timer_id_);
    loop_.retire(shared_from_this());  // 事件数组里的裸指针在本批结束前仍有效
    const int fd = fd_;
    manager_.remove(fd);
    ::close(fd);
    fd_ = -1;
    std::printf("closed fd=%d (%s)\n", fd, reason);
}

void ConnectionManager::accept_all(int listen_fd) {
    for (;;) {
        const int fd = ::accept4(listen_fd, nullptr, nullptr, SOCK_NONBLOCK | SOCK_CLOEXEC);
        if (fd < 0) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) return;
            if (errno == EINTR) continue;
            return;
        }
        auto conn = std::make_shared<Connection>(loop_, fd, *this);
        conns_.emplace(fd, conn);
        try {
            conn->start();
        } catch (...) {
            conns_.erase(fd);  // conn 的析构会 close(fd)
            throw;
        }
    }
}

void ConnectionManager::remove(int fd) { conns_.erase(fd); }
