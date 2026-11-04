// 36.6 的 epoll 版：同一个 kvd 协议，单线程 epoll + 非阻塞 socket。
// 为了让两版可比，这里只统计事件循环里的系统调用次数（epoll_wait、accept4、
// recv、send、epoll_ctl、close），不统计进程启动和内存分配。
#include <arpa/inet.h>
#include <sys/epoll.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <signal.h>
#include <sys/socket.h>
#include <unistd.h>

#include <cerrno>
#include <charconv>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <string>
#include <string_view>
#include <unordered_map>

#include "net.hpp"
#include "protocol.hpp"
#include "store.hpp"

namespace {

volatile sig_atomic_t g_stop = 0;
long g_syscalls = 0;

void on_signal(int) { g_stop = 1; }

struct Conn {
    Parser parser;
    std::string out;
};

void add_or_mod(int epfd, int fd, std::uint32_t events) {
    epoll_event ev{};
    ev.events = events;
    ev.data.fd = fd;
    ++g_syscalls;
    epoll_ctl(epfd, EPOLL_CTL_MOD, fd, &ev);
}

void close_conn(int epfd, int fd, std::unordered_map<int, Conn>& conns) {
    ++g_syscalls;
    epoll_ctl(epfd, EPOLL_CTL_DEL, fd, nullptr);
    ++g_syscalls;
    ::close(fd);
    conns.erase(fd);
}

// 返回 false 表示连接已关。
bool flush_out(int epfd, int fd, Conn& c, std::unordered_map<int, Conn>& conns) {
    while (!c.out.empty()) {
        ++g_syscalls;
        ssize_t n = ::send(fd, c.out.data(), c.out.size(), MSG_NOSIGNAL);
        if (n > 0) {
            c.out.erase(0, static_cast<std::size_t>(n));
            continue;
        }
        if (n < 0 && errno == EINTR) continue;
        if (n < 0 && (errno == EAGAIN || errno == EWOULDBLOCK)) {
            add_or_mod(epfd, fd, EPOLLIN | EPOLLOUT);
            return true;
        }
        close_conn(epfd, fd, conns);
        return false;
    }
    add_or_mod(epfd, fd, EPOLLIN);
    return true;
}

}  // namespace

int main(int argc, char** argv) {
    std::uint16_t port = 0;
    const char* port_file = nullptr;
    for (int i = 1; i < argc; ++i) {
        std::string_view arg = argv[i];
        auto next = [&]() -> const char* { return i + 1 < argc ? argv[++i] : nullptr; };
        if (arg == "--port") {
            const char* v = next();
            if (v == nullptr) return 2;
            unsigned p = 0;
            auto [ptr, ec] = std::from_chars(v, v + std::strlen(v), p);
            if (ec != std::errc{} || *ptr != '\0' || p > 65535) return 2;
            port = static_cast<std::uint16_t>(p);
        } else if (arg == "--port-file") {
            port_file = next();
            if (port_file == nullptr) return 2;
        } else {
            std::fprintf(stderr, "usage: %s [--port P] [--port-file F]\n", argv[0]);
            return 2;
        }
    }

    signal(SIGPIPE, SIG_IGN);
    struct sigaction sa {};
    sa.sa_handler = on_signal;
    sigemptyset(&sa.sa_mask);
    sigaction(SIGTERM, &sa, nullptr);
    sigaction(SIGINT, &sa, nullptr);

    try {
        int listener = listen_tcp(port, port_file);
        set_nonblocking(listener);
        int epfd = epoll_create1(EPOLL_CLOEXEC);
        if (epfd < 0) { perror("epoll_create1"); return 1; }
        epoll_event lev{};
        lev.events = EPOLLIN;
        lev.data.fd = listener;
        ++g_syscalls;
        if (epoll_ctl(epfd, EPOLL_CTL_ADD, listener, &lev) < 0) {
            perror("epoll_ctl");
            return 1;
        }

        Store store;
        std::unordered_map<int, Conn> conns;
        epoll_event events[64];
        while (!g_stop) {
            ++g_syscalls;
            int n = epoll_wait(epfd, events, 64, 100);
            if (n < 0) {
                if (errno == EINTR) continue;
                perror("epoll_wait");
                break;
            }
            for (int i = 0; i < n; ++i) {
                int fd = events[i].data.fd;
                if (fd == listener) {
                    for (;;) {
                        ++g_syscalls;
                        int conn = accept4(listener, nullptr, nullptr,
                                           SOCK_NONBLOCK | SOCK_CLOEXEC);
                        if (conn < 0) {
                            if (errno == EAGAIN || errno == EWOULDBLOCK) break;
                            if (errno == EINTR) continue;
                            break;
                        }
                        conns.emplace(conn, Conn{});
                        epoll_event cev{};
                        cev.events = EPOLLIN;
                        cev.data.fd = conn;
                        ++g_syscalls;
                        epoll_ctl(epfd, EPOLL_CTL_ADD, conn, &cev);
                    }
                    continue;
                }
                auto it = conns.find(fd);
                if (it == conns.end()) continue;
                Conn& c = it->second;

                if (events[i].events & EPOLLERR) {
                    close_conn(epfd, fd, conns);
                    continue;
                }
                if (events[i].events & EPOLLOUT) {
                    if (!flush_out(epfd, fd, c, conns)) continue;
                }
                if (!(events[i].events & EPOLLIN)) continue;

                bool closed = false;
                char buf[4096];
                for (;;) {
                    ++g_syscalls;
                    ssize_t r = recv(fd, buf, sizeof buf, 0);
                    if (r > 0) {
                        c.parser.feed(std::string_view(buf, static_cast<std::size_t>(r)));
                        while (auto result = c.parser.next()) {
                            if (auto* req = std::get_if<Request>(&*result)) {
                                c.out += format_response(store.apply(*req));
                            } else {
                                c.out += format_response(std::get<ParseError>(*result));
                            }
                        }
                        continue;
                    }
                    if (r == 0) {
                        closed = true;
                        break;
                    }
                    if (errno == EINTR) continue;
                    if (errno == EAGAIN || errno == EWOULDBLOCK) break;
                    closed = true;
                    break;
                }
                if (closed) {
                    close_conn(epfd, fd, conns);
                    continue;
                }
                if (!c.out.empty() && !flush_out(epfd, fd, c, conns)) continue;
                if (events[i].events & EPOLLHUP) close_conn(epfd, fd, conns);
            }
        }

        std::fprintf(stderr, "syscalls=%ld\n", g_syscalls);
        ++g_syscalls;
        ::close(epfd);
        ++g_syscalls;
        ::close(listener);
    } catch (const std::exception& e) {
        std::fprintf(stderr, "epoll_server: %s\n", e.what());
        return 1;
    }
    return 0;
}
