// kvd 的事件循环版：回调、eventfd 唤醒、最小堆空闲超时、signalfd 退出。
#include "event_loop.hpp"

#include "../../kvd/net.hpp"
#include "../../kvd/protocol.hpp"
#include "../../kvd/store.hpp"

#include <signal.h>
#include <sys/signalfd.h>
#include <sys/socket.h>
#include <unistd.h>

#include <cerrno>
#include <system_error>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <memory>
#include <string>
#include <string_view>
#include <thread>
#include <unordered_map>
#include <vector>

namespace {

constexpr std::size_t kHighWater = 4 * 1024 * 1024;
constexpr std::size_t kLowWater = 1024 * 1024;

class Server;

class CallbackHandler : public Handler {
public:
    CallbackHandler(int fd, std::function<void(std::uint32_t)> cb)
        : fd_(fd), cb_(std::move(cb)) {}
    int fd() const override { return fd_; }
    void handle_event(std::uint32_t events) override { cb_(events); }

private:
    int fd_;
    std::function<void(std::uint32_t)> cb_;
};

class Connection : public Handler, public std::enable_shared_from_this<Connection> {
public:
    Connection(EventLoop &loop, Server &server, int fd)
        : loop_(loop), server_(server), fd_(fd) {}
    ~Connection() override {
        if (fd_ >= 0) ::close(fd_);
    }

    int fd() const override { return fd_; }
    void handle_event(std::uint32_t events) override;
    void start();

private:
    void on_read();
    void flush();
    void reset_idle();
    void update_interest();
    void close(const char *reason);

    EventLoop &loop_;
    Server &server_;
    int fd_;
    Parser parser_;
    Buffer out_;
    std::uint64_t timer_id_ = 0;
    bool want_read_ = true;
    bool closed_ = false;
};

class Server : public Handler {
public:
    Server(std::uint16_t port, const char *port_file, int idle_seconds)
        : idle_seconds_(idle_seconds) {
        lfd_ = listen_tcp(port, port_file);
        set_nonblocking(lfd_);
        loop_.add(this, EPOLLIN);

        sigset_t mask;
        sigemptyset(&mask);
        sigaddset(&mask, SIGTERM);
        sigaddset(&mask, SIGINT);
        if (pthread_sigmask(SIG_BLOCK, &mask, nullptr) != 0)
            throw std::system_error(errno, std::generic_category(), "pthread_sigmask");
        sfd_ = ::signalfd(-1, &mask, SFD_NONBLOCK | SFD_CLOEXEC);
        if (sfd_ < 0)
            throw std::system_error(errno, std::generic_category(), "signalfd");
        signal_handler_ = std::make_unique<CallbackHandler>(
            sfd_, [this](std::uint32_t) { on_signal(); });
        loop_.add(signal_handler_.get(), EPOLLIN);
    }

    ~Server() override {
        conns_.clear();
        if (sfd_ >= 0) ::close(sfd_);
        if (lfd_ >= 0) ::close(lfd_);
    }

    int fd() const override { return lfd_; }
    void handle_event(std::uint32_t events) override {
        if (events & EPOLLIN) accept_all();
    }

    void run();
    EventLoop &loop() { return loop_; }
    Store &store() { return store_; }
    void remove_connection(int fd);
    int idle_seconds() const { return idle_seconds_; }

private:
    void accept_all();
    void on_signal();

    EventLoop loop_;
    std::jthread ticker_;  // 声明在 loop_ 之后：先停 ticker，再析构 loop_
    Store store_{16};
    int lfd_ = -1;
    int sfd_ = -1;
    int idle_seconds_ = 30;
    std::unique_ptr<CallbackHandler> signal_handler_;
    std::unordered_map<int, std::shared_ptr<Connection>> conns_;
    std::uint64_t served_ = 0;
    std::uint64_t ticks_ = 0;
    std::uint64_t closed_ = 0;
};

void Connection::start() {
    loop_.add(this, EPOLLIN | EPOLLRDHUP);
    reset_idle();
    std::printf("accepted fd=%d\n", fd_);
}

void Connection::reset_idle() {
    loop_.cancel_timer(timer_id_);
    std::weak_ptr<Connection> weak = weak_from_this();
    timer_id_ = loop_.add_timer(
        std::chrono::steady_clock::now() + std::chrono::seconds(server_.idle_seconds()),
        [weak] {
            if (auto self = weak.lock()) {
                std::printf("idle timeout fd=%d\n", self->fd_);
                self->timer_id_ = 0;
                self->close("idle timeout");
            }
        });
}

void Connection::update_interest() {
    std::uint32_t events = EPOLLRDHUP;
    if (want_read_) events |= EPOLLIN;
    if (!out_.empty()) events |= EPOLLOUT;
    loop_.mod(this, events);
}

void Connection::handle_event(std::uint32_t events) {
    if (events & (EPOLLERR | EPOLLHUP)) {
        close("error/hup");
        return;
    }
    if (events & EPOLLIN) on_read();
    if (!closed_ && (events & EPOLLOUT)) flush();
    if (!closed_ && (events & EPOLLRDHUP)) close("peer half-closed");
}

void Connection::on_read() {
    char buf[4096];
    bool got = false;
    for (;;) {
        const ssize_t n = ::recv(fd_, buf, sizeof buf, 0);
        if (n > 0) {
            got = true;
            parser_.feed(std::string_view(buf, static_cast<std::size_t>(n)));
            while (auto result = parser_.next()) {
                if (auto *req = std::get_if<Request>(&*result))
                    out_.append(format_response(server_.store().apply(*req)));
                else
                    out_.append(format_response(std::get<ParseError>(*result)));
            }
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
    // ANCHOR: backpressure
    if (got) {
        reset_idle();
        // 输出积压太多就暂停读：背压从源头传到对端，而不是把内存吃光。
        if (out_.size() > kHighWater) want_read_ = false;
    }
    // ANCHOR_END: backpressure
    flush();
}

void Connection::flush() {
    while (!out_.empty()) {
        const ssize_t n = ::send(fd_, out_.data().data(), out_.size(),
                                 MSG_NOSIGNAL | MSG_DONTWAIT);
        if (n > 0) {
            out_.consume(static_cast<std::size_t>(n));
            // ANCHOR: lowwater
            if (!want_read_ && out_.size() < kLowWater) want_read_ = true;
            // ANCHOR_END: lowwater
            continue;
        }
        if (n < 0 && errno == EINTR) continue;
        if (n < 0 && (errno == EAGAIN || errno == EWOULDBLOCK)) break;
        close("send error");
        return;
    }
    if (out_.empty()) want_read_ = true;
    update_interest();
}

void Connection::close(const char *reason) {
    if (closed_) return;
    closed_ = true;
    loop_.remove(this);
    loop_.cancel_timer(timer_id_);
    loop_.retire(shared_from_this());
    const int fd = fd_;
    server_.remove_connection(fd);
    ::close(fd);
    fd_ = -1;
    std::printf("closed fd=%d (%s)\n", fd, reason);
}

void Server::accept_all() {
    for (;;) {
        const int fd = ::accept4(lfd_, nullptr, nullptr, SOCK_NONBLOCK | SOCK_CLOEXEC);
        if (fd < 0) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) return;
            if (errno == EINTR) continue;
            return;
        }
        auto conn = std::make_shared<Connection>(loop_, *this, fd);
        conns_.emplace(fd, conn);
        ++served_;
        try {
            conn->start();
        } catch (...) {
            conns_.erase(fd);  // conn 的析构会 close(fd)
            throw;
        }
    }
}

void Server::remove_connection(int fd) {
    ++closed_;
    conns_.erase(fd);
}

void Server::on_signal() {
    signalfd_siginfo info{};
    while (::read(sfd_, &info, sizeof info) == static_cast<ssize_t>(sizeof info)) {
        std::printf("signal %u: 停止事件循环\n", info.ssi_signo);
        loop_.quit();
    }
}

void Server::run() {
    std::printf("server: listening, idle-timeout=%ds\n", idle_seconds_);
    // 先让循环线程就位，再启动投递任务的线程：run_in_loop 读 loop_thread_ 时没有竞争。
    loop_.run_in_loop([this] {
        ticker_ = std::jthread([this](std::stop_token stop) {
            while (!stop.stop_requested()) {
                std::this_thread::sleep_for(std::chrono::milliseconds(250));
                loop_.run_in_loop([this] { ++ticks_; });
            }
        });
    });
    loop_.run();
    ticker_.request_stop();
    ticker_.join();
    std::printf("server: connections=%llu ticks=%llu closed=%llu\n",
                static_cast<unsigned long long>(served_),
                static_cast<unsigned long long>(ticks_),
                static_cast<unsigned long long>(closed_));
}

std::uint16_t parse_port(const char *text) {
    char *end = nullptr;
    const unsigned long v = std::strtoul(text, &end, 10);
    if (end == text || *end != '\0' || v > 65535) {
        std::fprintf(stderr, "bad port: %s\n", text);
        std::exit(2);
    }
    return static_cast<std::uint16_t>(v);
}

}  // namespace

int main(int argc, char **argv) {
    std::uint16_t port = 0;
    const char *port_file = nullptr;
    int idle_seconds = 30;
    for (int i = 1; i < argc; ++i) {
        if (std::strcmp(argv[i], "--port") == 0 && i + 1 < argc) {
            port = parse_port(argv[++i]);
        } else if (std::strcmp(argv[i], "--port-file") == 0 && i + 1 < argc) {
            port_file = argv[++i];
        } else if (std::strcmp(argv[i], "--idle-timeout") == 0 && i + 1 < argc) {
            idle_seconds = std::atoi(argv[++i]);
        }
    }
    if (port == 0 && port_file == nullptr) {
        std::fprintf(stderr, "usage: %s (--port P | --port-file F) [--idle-timeout S]\n",
                     argv[0]);
        return 2;
    }
    signal(SIGPIPE, SIG_IGN);
    try {
        Server server(port, port_file, idle_seconds);
        server.run();
    } catch (const std::exception &e) {
        std::fprintf(stderr, "server: %s\n", e.what());
        return 1;
    }
    return 0;
}
