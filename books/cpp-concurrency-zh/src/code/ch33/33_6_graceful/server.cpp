// 优雅退出：SIGTERM 后不再 accept，等已经收下的请求写完响应再关连接。
#include "../../kvd/net.hpp"
#include "../../kvd/protocol.hpp"
#include "../../kvd/store.hpp"

#include <signal.h>
#include <sys/epoll.h>
#include <sys/signalfd.h>
#include <sys/socket.h>
#include <sys/timerfd.h>
#include <unistd.h>

#include <cerrno>
#include <chrono>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <string>

namespace {

class Server {
public:
    Server(const char *port_file, const char *ready_file) : ready_file_(ready_file) {
        lfd_ = listen_tcp(0, port_file);
        set_nonblocking(lfd_);

        sigset_t mask;
        sigemptyset(&mask);
        sigaddset(&mask, SIGTERM);
        sigaddset(&mask, SIGINT);
        if (pthread_sigmask(SIG_BLOCK, &mask, nullptr) != 0) throw std::system_error(errno, std::generic_category(), "pthread_sigmask");
        sfd_ = signalfd(-1, &mask, SFD_NONBLOCK | SFD_CLOEXEC);
        if (sfd_ < 0) throw std::system_error(errno, std::generic_category(), "signalfd");
        tfd_ = timerfd_create(CLOCK_MONOTONIC, TFD_NONBLOCK | TFD_CLOEXEC);
        ep_ = epoll_create1(EPOLL_CLOEXEC);
        if (tfd_ < 0 || ep_ < 0) throw std::system_error(errno, std::generic_category(), "timerfd/epoll");

        add(lfd_, EPOLLIN);
        add(sfd_, EPOLLIN);
        add(tfd_, EPOLLIN);
        store_.set("v", "hello");
        std::printf("server: listening (SIGTERM stops accept, then drains)\n");
    }

    ~Server() {
        if (cfd_ >= 0) ::close(cfd_);
        ::close(tfd_);
        ::close(sfd_);
        ::close(lfd_);
        ::close(ep_);
    }

    int run() {
        epoll_event events[16];
        while (!quit_) {
            const int n = epoll_wait(ep_, events, 16, -1);
            if (n < 0) continue;
            for (int i = 0; i < n; ++i) {
                const int fd = events[i].data.fd;
                if (fd == lfd_) accept_one();
                else if (fd == sfd_) on_signal();
                else if (fd == tfd_) on_timer();
                else on_conn(events[i].events);
            }
        }
        std::printf("server: drain done, exit\n");
        return 0;
    }

private:
    void add(int fd, std::uint32_t events) {
        epoll_event ev{};
        ev.events = events;
        ev.data.fd = fd;
        epoll_ctl(ep_, EPOLL_CTL_ADD, fd, &ev);
    }

    void accept_one() {
        for (;;) {
            const int fd = accept4(lfd_, nullptr, nullptr, SOCK_NONBLOCK | SOCK_CLOEXEC);
            if (fd < 0) break;
            if (cfd_ >= 0) {
                ::close(fd);  // 这个例子只演示一条连接
                continue;
            }
            cfd_ = fd;
            add(cfd_, EPOLLIN | EPOLLRDHUP);
            std::printf("accepted fd=%d\n", cfd_);
        }
    }

    void on_signal() {
        signalfd_siginfo info{};
        while (read(sfd_, &info, sizeof info) == static_cast<ssize_t>(sizeof info)) {
            std::printf("SIGTERM: stop accepting\n");
            stopping_ = true;
            epoll_ctl(ep_, EPOLL_CTL_DEL, lfd_, nullptr);
            maybe_finish();
        }
    }

    void on_timer() {
        std::uint64_t expirations = 0;
        const ssize_t r = read(tfd_, &expirations, sizeof expirations);
        (void)r;
        if (!pending_) return;
        pending_ = false;
        std::printf("pending request: response ready\n");
        out_.append(format_response(store_.apply(pending_request_)));
        flush();
    }

    void on_conn(std::uint32_t events) {
        if (events & (EPOLLERR | EPOLLHUP)) {
            close_conn("error/hup");
            return;
        }
        if (events & EPOLLIN) {
            char buf[1024];
            for (;;) {
                const ssize_t n = recv(cfd_, buf, sizeof buf, 0);
                if (n > 0) {
                    parser_.feed(std::string_view(buf, static_cast<std::size_t>(n)));
                    while (auto result = parser_.next()) {
                        if (auto *req = std::get_if<Request>(&*result)) {
                            pending_request_ = *req;
                            pending_ = true;
                            std::printf("request received, response delayed 500ms\n");
                            if (ready_file_ != nullptr && !notified_) {
                                std::ofstream(ready_file_) << "ready\n";
                                notified_ = true;
                            }
                            itimerspec its{};
                            its.it_value.tv_nsec = 500 * 1000 * 1000;
                            timerfd_settime(tfd_, 0, &its, nullptr);
                        } else {
                            out_.append(format_response(std::get<ParseError>(*result)));
                            flush();
                        }
                    }
                    continue;
                }
                if (n == 0) {
                    close_conn("peer closed");
                    return;
                }
                if (errno == EINTR) continue;
                if (errno == EAGAIN || errno == EWOULDBLOCK) break;
                close_conn("recv error");
                return;
            }
        }
        if (events & EPOLLOUT) flush();
        if (events & EPOLLRDHUP) close_conn("peer half-closed");
    }

    void flush() {
        if (cfd_ < 0) {
            maybe_finish();
            return;
        }
        while (!out_.empty()) {
            const ssize_t n = send(cfd_, out_.data(), out_.size(), MSG_NOSIGNAL | MSG_DONTWAIT);
            if (n > 0) {
                out_.erase(0, static_cast<std::size_t>(n));
                continue;
            }
            if (n < 0 && errno == EINTR) continue;
            if (n < 0 && (errno == EAGAIN || errno == EWOULDBLOCK)) {
                epoll_event ev{};
                ev.events = EPOLLIN | EPOLLRDHUP | EPOLLOUT;
                ev.data.fd = cfd_;
                epoll_ctl(ep_, EPOLL_CTL_MOD, cfd_, &ev);
                return;
            }
            close_conn("send error");
            return;
        }
        std::printf("response written\n");
        if (stopping_ && out_.empty()) close_conn("drained");
        else maybe_finish();
    }

    void close_conn(const char *why) {
        if (cfd_ < 0) return;
        epoll_ctl(ep_, EPOLL_CTL_DEL, cfd_, nullptr);
        ::close(cfd_);
        std::printf("closed fd=%d (%s)\n", cfd_, why);
        cfd_ = -1;
        maybe_finish();
    }

    void maybe_finish() {
        if (!stopping_) return;
        if (pending_) return;  // 还有请求没写完，继续跑循环
        if (cfd_ >= 0) {
            if (!out_.empty()) return;
            close_conn("drained");
            return;
        }
        quit_ = true;
    }

    int ep_ = -1;
    int lfd_ = -1;
    int sfd_ = -1;
    int tfd_ = -1;
    const char *ready_file_ = nullptr;
    bool notified_ = false;
    int cfd_ = -1;
    bool stopping_ = false;
    bool pending_ = false;
    bool quit_ = false;
    Parser parser_;
    Store store_{4};
    Request pending_request_;
    std::string out_;
};

}  // namespace

int main(int argc, char **argv) {
    const char *port_file = nullptr;
    const char *ready_file = nullptr;
    for (int i = 1; i < argc; ++i) {
        if (std::strcmp(argv[i], "--port-file") == 0 && i + 1 < argc) port_file = argv[++i];
        else if (std::strcmp(argv[i], "--ready-file") == 0 && i + 1 < argc) ready_file = argv[++i];
    }
    if (port_file == nullptr) return 2;
    std::setvbuf(stdout, nullptr, _IOLBF, 0);
    signal(SIGPIPE, SIG_IGN);
    try {
        Server server(port_file, ready_file);
        return server.run();
    } catch (const std::exception &e) {
        std::fprintf(stderr, "server: %s\n", e.what());
        return 1;
    }
}
