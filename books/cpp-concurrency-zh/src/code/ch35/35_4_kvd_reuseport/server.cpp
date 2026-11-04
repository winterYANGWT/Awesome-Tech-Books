// kvd_reuseport：N 个 worker，各自一个监听 socket（SO_REUSEPORT）和一个 epoll。
// 连接不跨 worker，也就没有跨线程的任务队列。
#include "net.hpp"
#include "protocol.hpp"
#include "store.hpp"

#include <arpa/inet.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <pthread.h>
#include <signal.h>
#include <sys/epoll.h>
#include <sys/signalfd.h>
#include <sys/socket.h>
#include <unistd.h>

#include <atomic>
#include <cerrno>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <memory>
#include <string>
#include <string_view>
#include <system_error>
#include <thread>
#include <unordered_map>
#include <vector>

namespace {

std::uint16_t parse_port(const char *text) {
    char *end = nullptr;
    unsigned long v = std::strtoul(text, &end, 10);
    if (end == text || *end != '\0' || v > 65535) {
        std::fprintf(stderr, "bad port: %s\n", text);
        std::exit(2);
    }
    return static_cast<std::uint16_t>(v);
}

void set_reuseport(int fd) {
    int yes = 1;
    if (setsockopt(fd, SOL_SOCKET, SO_REUSEPORT, &yes, sizeof yes) < 0)
        throw std::system_error(errno, std::generic_category(), "SO_REUSEPORT");
}

// 每个 worker 自己 bind/listen；port 为 0 时内核选一个，之后所有 worker 用同一个。
int make_listener(std::uint16_t port, std::uint16_t *actual) {
    int fd = socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK | SOCK_CLOEXEC, 0);
    if (fd < 0) throw std::system_error(errno, std::generic_category(), "socket");
    set_reuseport(fd);
    int yes = 1;
    if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof yes) < 0)
        throw std::system_error(errno, std::generic_category(), "SO_REUSEADDR");

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    if (bind(fd, reinterpret_cast<sockaddr *>(&addr), sizeof addr) < 0)
        throw std::system_error(errno, std::generic_category(), "bind");
    if (listen(fd, 128) < 0)
        throw std::system_error(errno, std::generic_category(), "listen");

    socklen_t len = sizeof addr;
    if (getsockname(fd, reinterpret_cast<sockaddr *>(&addr), &len) < 0)
        throw std::system_error(errno, std::generic_category(), "getsockname");
    *actual = ntohs(addr.sin_port);
    return fd;
}

void write_port_file(const char *path, std::uint16_t port) {
    const std::string tmp = std::string(path) + ".tmp." + std::to_string(getpid());
    int fd = open(tmp.c_str(), O_WRONLY | O_CREAT | O_TRUNC | O_CLOEXEC, 0644);
    if (fd < 0) throw std::system_error(errno, std::generic_category(), "open port file");
    const std::string text = std::to_string(port) + "\n";
    if (!write_all(fd, text)) {
        close(fd);
        unlink(tmp.c_str());
        throw std::system_error(errno, std::generic_category(), "write port file");
    }
    close(fd);
    if (rename(tmp.c_str(), path) < 0) {
        unlink(tmp.c_str());
        throw std::system_error(errno, std::generic_category(), "rename port file");
    }
}

struct Conn {
    int fd = -1;
    Parser parser;
    Buffer out;
};

class Worker {
public:
    Worker(std::size_t index, int listener, Store *store)
        : index_(index), listener_(listener), store_(store) {
        ep_ = epoll_create1(EPOLL_CLOEXEC);
        if (ep_ < 0)
            throw std::system_error(errno, std::generic_category(), "epoll_create1");
        epoll_event ev{};
        ev.events = EPOLLIN;
        ev.data.ptr = nullptr;
        if (epoll_ctl(ep_, EPOLL_CTL_ADD, listener_, &ev) != 0)
            throw std::system_error(errno, std::generic_category(), "epoll_ctl(listen)");
    }

    ~Worker() {
        for (auto &[fd, conn] : conns_) {
            (void)conn;
            close(fd);
        }
        close(ep_);
    }

    void run(const std::atomic<bool> *stop) {
        epoll_event events[64];
        while (!stop->load(std::memory_order_relaxed)) {
            const int n = epoll_wait(ep_, events, 64, 100);
            if (n < 0) {
                if (errno == EINTR) continue;
                break;
            }
            for (int i = 0; i < n; ++i) {
                if (events[i].data.ptr == nullptr) {
                    accept_all();
                } else {
                    handle(static_cast<Conn *>(events[i].data.ptr), events[i].events);
                }
            }
            graveyard_.clear();
        }
    }

    std::uint64_t served() const { return served_.load(); }

private:
    void accept_all() {
        for (;;) {
            int fd = accept4(listener_, nullptr, nullptr, SOCK_NONBLOCK | SOCK_CLOEXEC);
            if (fd < 0) {
                if (errno == EAGAIN || errno == EWOULDBLOCK) return;
                if (errno == EINTR) continue;
                return;
            }
            auto conn = std::make_unique<Conn>();
            conn->fd = fd;
            Conn *raw = conn.get();
            conns_.emplace(fd, std::move(conn));
            epoll_event ev{};
            ev.events = EPOLLIN | EPOLLRDHUP;
            ev.data.ptr = raw;
            if (epoll_ctl(ep_, EPOLL_CTL_ADD, fd, &ev) != 0) close_conn(raw);
        }
    }

    void handle(Conn *conn, std::uint32_t events) {
        if (events & (EPOLLERR | EPOLLHUP)) {
            close_conn(conn);
            return;
        }
        if (events & EPOLLIN) read_conn(conn);
        if (conn->fd < 0) return;
        if (events & (EPOLLIN | EPOLLOUT)) {
            flush(conn);
        }
        if (events & EPOLLRDHUP) close_conn(conn);
    }

    void read_conn(Conn *conn) {
        char buf[16384];
        for (;;) {
            const ssize_t n = recv(conn->fd, buf, sizeof buf, 0);
            if (n > 0) {
                conn->parser.feed(std::string_view(buf, static_cast<std::size_t>(n)));
                while (auto parsed = conn->parser.next()) {
                    if (auto *request = std::get_if<Request>(&*parsed)) {
                        conn->out.append(format_response(store_->apply(*request)));
                    } else {
                        conn->out.append(format_response(std::get<ParseError>(*parsed)));
                    }
                    served_.fetch_add(1, std::memory_order_relaxed);
                }
                continue;
            }
            if (n == 0) {
                close_conn(conn);
                return;
            }
            if (errno == EINTR) continue;
            if (errno == EAGAIN || errno == EWOULDBLOCK) return;
            close_conn(conn);
            return;
        }
    }

    void flush(Conn *conn) {
        while (!conn->out.empty()) {
            const ssize_t n = send(conn->fd, conn->out.data().data(), conn->out.size(),
                                   MSG_NOSIGNAL | MSG_DONTWAIT);
            if (n > 0) {
                conn->out.consume(static_cast<std::size_t>(n));
                continue;
            }
            if (n < 0 && errno == EINTR) continue;
            if (n < 0 && (errno == EAGAIN || errno == EWOULDBLOCK)) {
                epoll_event ev{};
                ev.events = EPOLLIN | EPOLLOUT | EPOLLRDHUP;
                ev.data.ptr = conn;
                epoll_ctl(ep_, EPOLL_CTL_MOD, conn->fd, &ev);
                return;
            }
            close_conn(conn);
            return;
        }
        epoll_event ev{};
        ev.events = EPOLLIN | EPOLLRDHUP;
        ev.data.ptr = conn;
        epoll_ctl(ep_, EPOLL_CTL_MOD, conn->fd, &ev);
    }

    void close_conn(Conn *conn) {
        const int fd = conn->fd;
        if (fd < 0) return;
        conn->fd = -1;
        epoll_ctl(ep_, EPOLL_CTL_DEL, fd, nullptr);
        auto it = conns_.find(fd);
        if (it != conns_.end()) {
            // 这一批事件里可能还有指向它的指针，先挪进墓地，循环结束再析构。
            graveyard_.push_back(std::move(it->second));
            conns_.erase(it);
        }
        close(fd);
    }

    std::size_t index_;
    int listener_ = -1;
    int ep_ = -1;
    Store *store_;
    std::unordered_map<int, std::unique_ptr<Conn>> conns_;
    std::vector<std::unique_ptr<Conn>> graveyard_;
    std::atomic<std::uint64_t> served_{0};
};

}  // namespace

int main(int argc, char **argv) {
    std::uint16_t port = 0;
    const char *port_file = nullptr;
    std::size_t workers = 4;
    for (int i = 1; i < argc; ++i) {
        if (std::strcmp(argv[i], "--port") == 0 && i + 1 < argc) {
            port = parse_port(argv[++i]);
        } else if (std::strcmp(argv[i], "--port-file") == 0 && i + 1 < argc) {
            port_file = argv[++i];
        } else if (std::strcmp(argv[i], "--workers") == 0 && i + 1 < argc) {
            workers = static_cast<std::size_t>(std::strtoul(argv[++i], nullptr, 10));
        } else {
            std::fprintf(stderr, "usage: %s (--port P | --port-file F) [--workers N]\n",
                         argv[0]);
            return 2;
        }
    }
    if (workers == 0) workers = 1;

    signal(SIGPIPE, SIG_IGN);

    sigset_t mask;
    sigemptyset(&mask);
    sigaddset(&mask, SIGTERM);
    sigaddset(&mask, SIGINT);
    if (pthread_sigmask(SIG_BLOCK, &mask, nullptr) != 0) {
        std::fprintf(stderr, "pthread_sigmask failed\n");
        return 1;
    }
    const int sig_fd = signalfd(-1, &mask, SFD_CLOEXEC);
    if (sig_fd < 0) {
        std::fprintf(stderr, "signalfd: %s\n", std::strerror(errno));
        return 1;
    }

    Store store(64);
    std::vector<int> listeners;
    std::vector<std::unique_ptr<Worker>> pool;
    std::atomic<bool> stop{false};
    try {
        std::uint16_t actual = 0;
        for (std::size_t i = 0; i < workers; ++i) {
            const std::uint16_t bind_port = (i == 0) ? port : actual;
            listeners.push_back(make_listener(bind_port, &actual));
        }
        if (port_file != nullptr) write_port_file(port_file, actual);

        for (std::size_t i = 0; i < workers; ++i) {
            pool.push_back(std::make_unique<Worker>(i, listeners[i], &store));
        }
    } catch (const std::exception &e) {
        std::fprintf(stderr, "server: %s\n", e.what());
        return 1;
    }

    std::vector<std::jthread> threads;
    threads.reserve(workers);
    for (std::size_t i = 0; i < workers; ++i) {
        threads.emplace_back([&, i] { pool[i]->run(&stop); });
    }

    signalfd_siginfo info{};
    while (read(sig_fd, &info, sizeof info) != sizeof info) {
        if (errno != EINTR) break;
    }
    stop.store(true);
    threads.clear();
    close(sig_fd);

    std::printf("reuseport: workers=%zu\n", workers);
    for (std::size_t i = 0; i < pool.size(); ++i) {
        std::printf("reuseport: worker %zu requests=%llu\n", i,
                    static_cast<unsigned long long>(pool[i]->served()));
    }
    for (int fd : listeners) close(fd);
    return 0;
}
