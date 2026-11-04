// timerfd 周期检查：3 秒没有请求的连接关掉，持续发请求的连接留着。
#include "../../kvd/net.hpp"

#include <sys/epoll.h>
#include <sys/socket.h>
#include <sys/timerfd.h>
#include <unistd.h>

#include <cerrno>
#include <chrono>
#include <cstdio>
#include <cstring>
#include <string>
#include <unordered_map>
#include <vector>

namespace {

using Clock = std::chrono::steady_clock;
constexpr auto kIdle = std::chrono::seconds(3);

struct Conn {
    int fd = -1;
    Clock::time_point last_active;
    std::string in;
};

void close_conn(int ep, std::unordered_map<int, Conn> &conns, int fd, const char *why) {
    auto it = conns.find(fd);
    if (it == conns.end()) return;
    epoll_ctl(ep, EPOLL_CTL_DEL, fd, nullptr);
    ::close(fd);
    std::printf("close fd=%d (%s)\n", fd, why);
    conns.erase(it);
}

}  // namespace

int main(int argc, char **argv) {
    const char *port_file = nullptr;
    for (int i = 1; i < argc; ++i)
        if (std::strcmp(argv[i], "--port-file") == 0 && i + 1 < argc) port_file = argv[++i];
    if (port_file == nullptr) return 2;

    std::setvbuf(stdout, nullptr, _IOLBF, 0);
    const int lfd = listen_tcp(0, port_file);
    set_nonblocking(lfd);
    const int ep = epoll_create1(EPOLL_CLOEXEC);
    const int tfd = timerfd_create(CLOCK_MONOTONIC, TFD_NONBLOCK | TFD_CLOEXEC);

    epoll_event ev{};
    ev.events = EPOLLIN;
    ev.data.fd = lfd;
    epoll_ctl(ep, EPOLL_CTL_ADD, lfd, &ev);
    ev.data.fd = tfd;
    epoll_ctl(ep, EPOLL_CTL_ADD, tfd, &ev);

    itimerspec its{};
    its.it_value.tv_nsec = 500 * 1000 * 1000;
    its.it_interval.tv_nsec = 500 * 1000 * 1000;
    timerfd_settime(tfd, 0, &its, nullptr);

    std::unordered_map<int, Conn> conns;
    std::printf("server: listening, idle=3s\n");

    for (;;) {
        epoll_event events[16];
        const int n = epoll_wait(ep, events, 16, -1);
        if (n < 0) continue;
        for (int i = 0; i < n; ++i) {
            const int fd = events[i].data.fd;
            if (fd == lfd) {
                for (;;) {
                    const int c = accept4(lfd, nullptr, nullptr, SOCK_NONBLOCK | SOCK_CLOEXEC);
                    if (c < 0) break;
                    conns.emplace(c, Conn{c, Clock::now(), {}});
                    epoll_event ce{};
                    ce.events = EPOLLIN;
                    ce.data.fd = c;
                    epoll_ctl(ep, EPOLL_CTL_ADD, c, &ce);
                    std::printf("accepted fd=%d\n", c);
                }
            } else if (fd == tfd) {
                std::uint64_t ticks = 0;
                const ssize_t r = read(tfd, &ticks, sizeof ticks);
                (void)r;
                const auto now = Clock::now();
                std::vector<int> expired;
                for (const auto &[cfd, conn] : conns)
                    if (now - conn.last_active > kIdle) expired.push_back(cfd);
                for (int cfd : expired) close_conn(ep, conns, cfd, "idle timeout");
            } else {
                auto it = conns.find(fd);
                if (it == conns.end()) continue;
                char buf[512];
                for (;;) {
                    const ssize_t r = recv(fd, buf, sizeof buf, 0);
                    if (r > 0) {
                        it->second.in.append(buf, static_cast<std::size_t>(r));
                        it->second.last_active = Clock::now();
                        std::size_t pos;
                        while ((pos = it->second.in.find('\n')) != std::string::npos) {
                            it->second.in.erase(0, pos + 1);
                            const char pong[] = "PONG\n";
                            const ssize_t s = send(fd, pong, 5, MSG_NOSIGNAL | MSG_DONTWAIT);
                            (void)s;
                            std::printf("pong fd=%d\n", fd);
                        }
                        continue;
                    }
                    if (r == 0) {
                        close_conn(ep, conns, fd, "peer closed");
                        break;
                    }
                    if (errno == EINTR) continue;
                    if (errno == EAGAIN || errno == EWOULDBLOCK) break;
                    close_conn(ep, conns, fd, "recv error");
                    break;
                }
            }
        }
    }
}
