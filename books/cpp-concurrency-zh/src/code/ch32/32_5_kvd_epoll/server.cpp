// kvd 的单线程 epoll 版：--mode lt 或 et。连接对象放在 data.ptr，
// 每条连接有自己的解析器、输入缓冲和输出缓冲。
#include "net.hpp"
#include "protocol.hpp"
#include "store.hpp"

#include <signal.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <unistd.h>

#include <cerrno>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <memory>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace {
volatile sig_atomic_t g_stop = 0;
void on_term(int) { g_stop = 1; }

std::uint16_t parse_port(const char *text) {
    char *end = nullptr;
    unsigned long v = std::strtoul(text, &end, 10);
    if (end == text || *end != '\0' || v > 65535) {
        std::fprintf(stderr, "bad port: %s\n", text);
        std::exit(2);
    }
    return static_cast<std::uint16_t>(v);
}

struct Conn {
    int fd = -1;
    Parser parser;
    Buffer out;
    bool want_write = false;
};

class Server {
public:
    Server(bool et, Store *store) : et_(et), store_(store) {
        ep_ = epoll_create1(EPOLL_CLOEXEC);
        if (ep_ < 0) throw std::system_error(errno, std::generic_category(), "epoll_create1");
    }
    ~Server() {
        for (auto &[fd, c] : conns_) {
            (void)c;
            epoll_ctl(ep_, EPOLL_CTL_DEL, fd, nullptr);
            close(fd);
        }
        close(ep_);
    }

    void add_listener(int lfd) {
        lfd_ = lfd;
        epoll_event ev{};
        ev.events = EPOLLIN | (et_ ? static_cast<std::uint32_t>(EPOLLET) : 0u);
        ev.data.ptr = nullptr;  // nullptr 表示监听 fd
        if (epoll_ctl(ep_, EPOLL_CTL_ADD, lfd, &ev) != 0)
            throw std::system_error(errno, std::generic_category(), "epoll_ctl(listen)");
    }

    void run() {
        epoll_event out[64];
        while (!g_stop) {
            int n = epoll_wait(ep_, out, 64, 200);
            if (n < 0) {
                if (errno == EINTR) continue;
                throw std::system_error(errno, std::generic_category(), "epoll_wait");
            }
            for (int i = 0; i < n; ++i) {
                if (out[i].data.ptr == nullptr) {
                    accept_all();
                    continue;
                }
                Conn *c = static_cast<Conn *>(out[i].data.ptr);
                if (out[i].events & (EPOLLERR | EPOLLHUP)) {
                    close_conn(c);
                    continue;
                }
                if (out[i].events & (EPOLLIN | EPOLLRDHUP)) read_conn(c);
                if (c->fd < 0) continue;
                if (out[i].events & EPOLLOUT) flush(c);
                if ((out[i].events & EPOLLRDHUP) && c->fd >= 0) close_conn(c);
            }
            graveyard_.clear();  // 这一批事件处理完，才真正析构关闭的连接
        }
    }

    std::uint64_t served() const { return served_; }

private:
// ANCHOR: accept
    void accept_all() {
        for (;;) {
            int fd = accept4(lfd_, nullptr, nullptr, SOCK_NONBLOCK | SOCK_CLOEXEC);
            if (fd < 0) {
                if (errno == EAGAIN || errno == EWOULDBLOCK) return;
                if (errno == EINTR) continue;
                perror("accept4");
                return;
            }
            auto c = std::make_unique<Conn>();
            c->fd = fd;
            Conn *raw = c.get();
            conns_.emplace(fd, raw);
            owners_.emplace(fd, std::move(c));
            ++served_;
            epoll_event ev{};
            ev.events = EPOLLIN | EPOLLRDHUP | (et_ ? static_cast<std::uint32_t>(EPOLLET) : 0u);
            ev.data.ptr = raw;
            if (epoll_ctl(ep_, EPOLL_CTL_ADD, fd, &ev) != 0) {
                close_conn(raw);
            }
        }
    }

// ANCHOR_END: accept
// ANCHOR: read
    void read_conn(Conn *c) {
        char buf[4096];
        for (;;) {
            ssize_t n = recv(c->fd, buf, sizeof buf, 0);
            if (n > 0) {
                c->parser.feed(std::string_view(buf, static_cast<std::size_t>(n)));
                while (auto result = c->parser.next()) {
                    if (auto *req = std::get_if<Request>(&*result))
                        c->out.append(format_response(store_->apply(*req)));
                    else
                        c->out.append(format_response(std::get<ParseError>(*result)));
                }
                continue;
            }
            if (n == 0) {
                close_conn(c);
                return;
            }
            if (errno == EINTR) continue;
            if (errno == EAGAIN || errno == EWOULDBLOCK) break;
            close_conn(c);
            return;
        }
        flush(c);
    }

// ANCHOR_END: read
    void flush(Conn *c) {
        while (!c->out.empty()) {
            ssize_t n = send(c->fd, c->out.data().data(), c->out.size(),
                             MSG_NOSIGNAL | MSG_DONTWAIT);
            if (n > 0) {
                c->out.consume(static_cast<std::size_t>(n));
                continue;
            }
            if (n < 0 && errno == EINTR) continue;
            if (n < 0 && (errno == EAGAIN || errno == EWOULDBLOCK)) break;
            close_conn(c);
            return;
        }
        bool want = !c->out.empty();
        if (want != c->want_write) {
            c->want_write = want;
            epoll_event ev{};
            ev.events = EPOLLIN | EPOLLRDHUP |
                        (et_ ? static_cast<std::uint32_t>(EPOLLET) : 0u) |
                        (want ? static_cast<std::uint32_t>(EPOLLOUT) : 0u);
            ev.data.ptr = c;
            epoll_ctl(ep_, EPOLL_CTL_MOD, c->fd, &ev);
        }
    }

    void close_conn(Conn *c) {
        if (c->fd < 0) return;
        int fd = c->fd;
        c->fd = -1;
        epoll_ctl(ep_, EPOLL_CTL_DEL, fd, nullptr);
        conns_.erase(fd);
        auto it = owners_.find(fd);
        if (it != owners_.end()) {
            graveyard_.push_back(std::move(it->second));
            owners_.erase(it);
        }
        close(fd);
    }

    int ep_ = -1;
    int lfd_ = -1;
    bool et_ = false;
    Store *store_ = nullptr;
    std::uint64_t served_ = 0;
    std::unordered_map<int, Conn *> conns_;
    std::unordered_map<int, std::unique_ptr<Conn>> owners_;
    std::vector<std::unique_ptr<Conn>> graveyard_;
};
}  // namespace

int main(int argc, char **argv) {
    std::uint16_t port = 0;
    const char *port_file = nullptr;
    bool et = false;
    for (int i = 1; i < argc; ++i) {
        if (std::strcmp(argv[i], "--port") == 0 && i + 1 < argc) {
            port = parse_port(argv[++i]);
        } else if (std::strcmp(argv[i], "--port-file") == 0 && i + 1 < argc) {
            port_file = argv[++i];
        } else if (std::strcmp(argv[i], "--mode") == 0 && i + 1 < argc) {
            ++i;
            if (std::strcmp(argv[i], "et") == 0) et = true;
            else if (std::strcmp(argv[i], "lt") != 0) {
                std::fprintf(stderr, "--mode must be lt or et\n");
                return 2;
            }
        }
    }
    if (port == 0 && port_file == nullptr) {
        std::fprintf(stderr, "usage: %s (--port P | --port-file F) [--mode lt|et]\n", argv[0]);
        return 2;
    }

    struct sigaction sa;
    std::memset(&sa, 0, sizeof sa);
    sa.sa_handler = on_term;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    sigaction(SIGTERM, &sa, nullptr);
    sigaction(SIGINT, &sa, nullptr);
    signal(SIGPIPE, SIG_IGN);

    Store store(16);
    try {
        Server server(et, &store);
        int lfd = listen_tcp(port, port_file);
        set_nonblocking(lfd);  // ET 下要 accept 到 EAGAIN，监听 fd 必须非阻塞
        server.add_listener(lfd);
        server.run();
        std::printf("server: mode=%s served=%llu\n", et ? "et" : "lt",
                    (unsigned long long)server.served());
        close(lfd);
    } catch (const std::exception &e) {
        std::fprintf(stderr, "server: %s\n", e.what());
        return 1;
    }
    return 0;
}
