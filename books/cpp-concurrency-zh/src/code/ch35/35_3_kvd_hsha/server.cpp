// kvd_hsha：1 个 I/O 线程 + N 个 worker + 有界请求队列。
// I/O 线程只做 accept/read/write；完整的一行请求入队，worker 解析并查表，
// 响应经结果队列和 eventfd 回到 I/O 线程写出。
#include "net.hpp"
#include "protocol.hpp"
#include "store.hpp"

#include <arpa/inet.h>
#include <pthread.h>
#include <signal.h>
#include <sys/epoll.h>
#include <sys/eventfd.h>
#include <sys/signalfd.h>
#include <sys/socket.h>
#include <unistd.h>

#include <atomic>
#include <cerrno>
#include <condition_variable>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <deque>
#include <memory>
#include <mutex>
#include <optional>
#include <string>
#include <string_view>
#include <system_error>
#include <thread>
#include <unordered_map>
#include <utility>
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

struct Conn {
    int fd = -1;
    Buffer in;
    Buffer out;
    bool busy = false;        // 已有一个任务在处理这条连接
    bool peer_closed = false;
    bool closed = false;
};

struct Task {
    std::shared_ptr<Conn> conn;
    std::string requests;  // 一批完整的行，每行以 '\n' 结尾
};

struct Result {
    std::shared_ptr<Conn> conn;
    std::string response;
};

// ANCHOR: queue
// 有界队列：满了 try_push 返回 false，由 I/O 线程暂停读，形成背压。
class TaskQueue {
public:
    explicit TaskQueue(std::size_t cap) : cap_(cap) {}
    TaskQueue(const TaskQueue &) = delete;
    TaskQueue &operator=(const TaskQueue &) = delete;

    bool try_push(Task task) {
        std::lock_guard<std::mutex> lock(mu_);
        if (closed_ || tasks_.size() >= cap_) return false;
        tasks_.push_back(std::move(task));
        not_empty_.notify_one();
        return true;
    }

    std::optional<Task> pop() {
        std::unique_lock<std::mutex> lock(mu_);
        not_empty_.wait(lock, [this] { return closed_ || !tasks_.empty(); });
        if (tasks_.empty()) return std::nullopt;
        Task task = std::move(tasks_.front());
        tasks_.pop_front();
        return task;
    }

    std::size_t size() const {
        std::lock_guard<std::mutex> lock(mu_);
        return tasks_.size();
    }

    void close() {
        std::lock_guard<std::mutex> lock(mu_);
        closed_ = true;
        not_empty_.notify_all();
    }

private:
    mutable std::mutex mu_;
    std::condition_variable not_empty_;
    std::deque<Task> tasks_;
    std::size_t cap_;
    bool closed_ = false;
};
// ANCHOR_END: queue

class Server {
public:
    Server(std::size_t workers, std::size_t queue_cap, int listen_fd)
        : workers_(workers), queue_cap_(queue_cap), listen_fd_(listen_fd), queue_(queue_cap) {
        ep_ = epoll_create1(EPOLL_CLOEXEC);
        if (ep_ < 0)
            throw std::system_error(errno, std::generic_category(), "epoll_create1");

        wake_fd_ = eventfd(0, EFD_NONBLOCK | EFD_CLOEXEC);
        if (wake_fd_ < 0)
            throw std::system_error(errno, std::generic_category(), "eventfd");

        sigset_t mask;
        sigemptyset(&mask);
        sigaddset(&mask, SIGTERM);
        sigaddset(&mask, SIGINT);
        const int mask_rc = pthread_sigmask(SIG_BLOCK, &mask, nullptr);
        if (mask_rc != 0)
            throw std::system_error(mask_rc, std::generic_category(), "pthread_sigmask");
        sig_fd_ = signalfd(-1, &mask, SFD_NONBLOCK | SFD_CLOEXEC);
        if (sig_fd_ < 0)
            throw std::system_error(errno, std::generic_category(), "signalfd");

        add_source(listen_fd_, &listen_tag_, EPOLLIN);
        add_source(wake_fd_, &wake_tag_, EPOLLIN);
        add_source(sig_fd_, &sig_tag_, EPOLLIN);

        for (std::size_t i = 0; i < workers_; ++i) worker_served_.emplace_back(0);
        for (std::size_t i = 0; i < workers_; ++i) {
            threads_.emplace_back([this, i] { worker_loop(i); });
        }
    }

    ~Server() {
        for (auto &[fd, conn] : conns_) {
            (void)conn;
            if (fd >= 0) close(fd);
        }
        queue_.close();
        threads_.clear();  // jthread 析构：request_stop 再 join
        if (sig_fd_ >= 0) close(sig_fd_);
        if (wake_fd_ >= 0) close(wake_fd_);
        if (ep_ >= 0) close(ep_);
    }

    void run() {
        epoll_event events[64];
        while (!stop_) {
            const int n = epoll_wait(ep_, events, 64, 200);
            if (n < 0) {
                if (errno == EINTR) continue;
                throw std::system_error(errno, std::generic_category(), "epoll_wait");
            }
            for (int i = 0; i < n; ++i) {
                void *ptr = events[i].data.ptr;
                if (ptr == &listen_tag_) {
                    accept_all();
                } else if (ptr == &wake_tag_) {
                    drain_wakeup();
                    collect_results();
                } else if (ptr == &sig_tag_) {
                    drain_signals();
                } else {
                    auto *conn = static_cast<Conn *>(ptr);
                    handle_conn(conn, events[i].events);
                }
            }
            graveyard_.clear();  // 这一批事件处理完再析构关闭的连接
            collect_results();
        }
        for (auto &[fd, conn] : conns_) {
            (void)conn;
            if (fd >= 0) close(fd);
        }
        conns_.clear();
        queue_.close();
        threads_.clear();  // 先 join：worker 可能还在处理已经取走的任务
        std::printf("hsha: workers=%zu queue_cap=%zu requests=%llu\n", workers_,
                    queue_cap_, static_cast<unsigned long long>(served_.load()));
        for (std::size_t i = 0; i < worker_served_.size(); ++i) {
            std::printf("hsha: worker %zu served=%llu\n", i,
                        static_cast<unsigned long long>(worker_served_[i].load()));
        }
    }

private:
    struct Tag {
        int kind;
    };

    void add_source(int fd, Tag *tag, std::uint32_t events) {
        epoll_event ev{};
        ev.events = events;
        ev.data.ptr = tag;
        if (epoll_ctl(ep_, EPOLL_CTL_ADD, fd, &ev) != 0)
            throw std::system_error(errno, std::generic_category(), "epoll_ctl(add)");
    }

    void accept_all() {
        for (;;) {
            int fd = accept4(listen_fd_, nullptr, nullptr, SOCK_NONBLOCK | SOCK_CLOEXEC);
            if (fd < 0) {
                if (errno == EAGAIN || errno == EWOULDBLOCK) return;
                if (errno == EINTR) continue;
                return;
            }
            auto conn = std::make_shared<Conn>();
            conn->fd = fd;
            Conn *raw = conn.get();
            conns_.emplace(fd, std::move(conn));
            epoll_event ev{};
            ev.events = EPOLLIN | EPOLLRDHUP;
            ev.data.ptr = raw;
            if (epoll_ctl(ep_, EPOLL_CTL_ADD, fd, &ev) != 0) {
                close_conn(raw);
            }
        }
    }

    void handle_conn(Conn *conn, std::uint32_t events) {
        if (conn->closed) return;
        if (events & (EPOLLERR | EPOLLHUP)) {
            close_conn(conn);
            return;
        }
        if (events & EPOLLIN) read_conn(conn);
        if (conn->closed) return;
        if (events & EPOLLOUT) flush(conn);
        if (conn->closed) return;
        if (events & EPOLLRDHUP) {
            conn->peer_closed = true;
            maybe_close(conn);
        }
        if (!conn->closed) {
            dispatch_next(conn);
            mod_conn(conn);
        }
    }

    void read_conn(Conn *conn) {
        char buf[16384];
        for (;;) {
            const ssize_t n = recv(conn->fd, buf, sizeof buf, 0);
            if (n > 0) {
                conn->in.append(std::string_view(buf, static_cast<std::size_t>(n)));
                continue;
            }
            if (n == 0) {
                conn->peer_closed = true;
                return;
            }
            if (errno == EINTR) continue;
            if (errno == EAGAIN || errno == EWOULDBLOCK) return;
            close_conn(conn);
            return;
        }
    }

// ANCHOR: dispatch
    // 取出一批完整的行，保持连接内的请求顺序；没有整行时返回 false。
    static bool take_batch(Conn *conn, std::string *batch) {
        const std::string_view data = conn->in.data();
        constexpr int kMaxBatch = 16;
        std::size_t start = 0;
        int lines = 0;
        while (lines < kMaxBatch) {
            const std::size_t pos = data.find('\n', start);
            if (pos == std::string_view::npos) break;
            batch->append(data.substr(start, pos - start));
            batch->push_back('\n');
            start = pos + 1;
            ++lines;
        }
        if (lines == 0) return false;
        conn->in.consume(start);
        return true;
    }

    void dispatch_next(Conn *conn) {
        if (conn->closed || conn->busy || paused_) return;
        if (queue_.size() >= queue_cap_) {
            pause_reads();
            return;
        }
        std::string batch;
        if (!take_batch(conn, &batch)) {
            maybe_close(conn);
            return;
        }
        auto owner = conns_.find(conn->fd);
        if (owner == conns_.end()) return;
        Task task{owner->second, std::move(batch)};
        if (!queue_.try_push(std::move(task))) {
            // 只有 I/O 线程 push，worker 只 pop；这里仍然保守地暂停。
            pause_reads();
            return;
        }
        conn->busy = true;
    }
// ANCHOR_END: dispatch

    void flush(Conn *conn) {
        while (!conn->out.empty()) {
            const ssize_t n = send(conn->fd, conn->out.data().data(), conn->out.size(),
                                   MSG_NOSIGNAL | MSG_DONTWAIT);
            if (n > 0) {
                conn->out.consume(static_cast<std::size_t>(n));
                continue;
            }
            if (n < 0 && errno == EINTR) continue;
            if (n < 0 && (errno == EAGAIN || errno == EWOULDBLOCK)) break;
            close_conn(conn);
            return;
        }
    }

    void mod_conn(Conn *conn) {
        if (conn->closed) return;
        std::uint32_t events = EPOLLRDHUP;
        if (!conn->busy && !paused_) events |= EPOLLIN;
        if (!conn->out.empty()) events |= EPOLLOUT;
        epoll_event ev{};
        ev.events = events;
        ev.data.ptr = conn;
        epoll_ctl(ep_, EPOLL_CTL_MOD, conn->fd, &ev);
    }

    void maybe_close(Conn *conn) {
        if (!conn->peer_closed || conn->busy || !conn->out.empty()) return;
        if (conn->in.data().find('\n') != std::string_view::npos) return;
        close_conn(conn);
    }

    void close_conn(Conn *conn) {
        if (conn->closed) return;
        conn->closed = true;
        const int fd = conn->fd;
        conn->fd = -1;
        if (fd >= 0) {
            epoll_ctl(ep_, EPOLL_CTL_DEL, fd, nullptr);
            auto it = conns_.find(fd);
            if (it != conns_.end()) {
                graveyard_.push_back(std::move(it->second));
                conns_.erase(it);
            }
            close(fd);
        }
    }

    void drain_wakeup() {
        std::uint64_t value = 0;
        while (read(wake_fd_, &value, sizeof value) > 0) {
        }
    }

    void collect_results() {
        std::deque<Result> ready;
        {
            std::lock_guard<std::mutex> lock(result_mu_);
            ready.swap(results_);
        }
        for (auto &r : ready) {
            Conn *conn = r.conn.get();
            if (!conn->closed) {
                conn->out.append(r.response);
                conn->busy = false;
                flush(conn);
                if (!conn->closed) {
                    dispatch_next(conn);
                    mod_conn(conn);
                }
            }
            if (paused_ && queue_.size() < queue_cap_ / 2) resume_reads();
        }
    }

    void pause_reads() {
        if (paused_) return;
        paused_ = true;
        for (auto &[fd, conn] : conns_) {
            (void)fd;
            mod_conn(conn.get());
        }
        epoll_event ev{};
        ev.events = 0;
        ev.data.ptr = &listen_tag_;
        epoll_ctl(ep_, EPOLL_CTL_MOD, listen_fd_, &ev);
    }

    void resume_reads() {
        if (!paused_) return;
        paused_ = false;
        epoll_event ev{};
        ev.events = EPOLLIN;
        ev.data.ptr = &listen_tag_;
        epoll_ctl(ep_, EPOLL_CTL_MOD, listen_fd_, &ev);
        // dispatch_next 可能因对端关闭而 close_conn、从 conns_ 里 erase；
        // 先拷一份 shared_ptr，避免遍历中当前迭代器失效。
        std::vector<std::shared_ptr<Conn>> snapshot;
        snapshot.reserve(conns_.size());
        for (auto &[fd, conn] : conns_) {
            (void)fd;
            snapshot.push_back(conn);
        }
        for (auto &conn : snapshot) {
            dispatch_next(conn.get());
            mod_conn(conn.get());
        }
    }

    void drain_signals() {
        signalfd_siginfo info{};
        while (read(sig_fd_, &info, sizeof info) == sizeof info) {
            if (info.ssi_signo == SIGTERM || info.ssi_signo == SIGINT) stop_ = true;
        }
    }

// ANCHOR: worker
    void worker_loop(std::size_t index) {
        while (auto task = queue_.pop()) {
            std::string response;
            std::uint64_t parsed_count = 0;
            Parser parser;
            parser.feed(task->requests);
            while (auto parsed = parser.next()) {
                if (auto *request = std::get_if<Request>(&*parsed)) {
                    response += format_response(store_.apply(*request));
                } else {
                    response += format_response(std::get<ParseError>(*parsed));
                }
                ++parsed_count;
            }
            {
                std::lock_guard<std::mutex> lock(result_mu_);
                results_.push_back(Result{task->conn, std::move(response)});
            }
            const std::uint64_t one = 1;
            const ssize_t wake_rc = write(wake_fd_, &one, sizeof one);
            if (wake_rc < 0 && errno != EAGAIN) {
                std::fprintf(stderr, "worker: eventfd write: %s\n", std::strerror(errno));
            }
            served_.fetch_add(parsed_count, std::memory_order_relaxed);
            worker_served_[index].fetch_add(parsed_count, std::memory_order_relaxed);
        }
    }
// ANCHOR_END: worker

    std::size_t workers_;
    std::size_t queue_cap_;
    int listen_fd_ = -1;
    int ep_ = -1;
    int wake_fd_ = -1;
    int sig_fd_ = -1;
    Tag listen_tag_{0};
    Tag wake_tag_{1};
    Tag sig_tag_{2};

    Store store_{256};
    TaskQueue queue_;
    std::mutex result_mu_;
    std::deque<Result> results_;
    std::unordered_map<int, std::shared_ptr<Conn>> conns_;
    std::vector<std::shared_ptr<Conn>> graveyard_;
    std::vector<std::jthread> threads_;
    std::deque<std::atomic<std::uint64_t>> worker_served_;
    std::atomic<std::uint64_t> served_{0};
    bool paused_ = false;
    bool stop_ = false;
};

}  // namespace

int main(int argc, char **argv) {
    std::uint16_t port = 0;
    const char *port_file = nullptr;
    std::size_t workers = 8;
    std::size_t queue_cap = 1024;
    for (int i = 1; i < argc; ++i) {
        if (std::strcmp(argv[i], "--port") == 0 && i + 1 < argc) {
            port = parse_port(argv[++i]);
        } else if (std::strcmp(argv[i], "--port-file") == 0 && i + 1 < argc) {
            port_file = argv[++i];
        } else if (std::strcmp(argv[i], "--workers") == 0 && i + 1 < argc) {
            workers = static_cast<std::size_t>(std::strtoul(argv[++i], nullptr, 10));
        } else if (std::strcmp(argv[i], "--queue-cap") == 0 && i + 1 < argc) {
            queue_cap = static_cast<std::size_t>(std::strtoul(argv[++i], nullptr, 10));
        } else {
            std::fprintf(stderr,
                         "usage: %s (--port P | --port-file F) [--workers N] "
                         "[--queue-cap N]\n",
                         argv[0]);
            return 2;
        }
    }
    if (workers == 0) workers = 1;
    if (queue_cap < 2) queue_cap = 2;

    signal(SIGPIPE, SIG_IGN);
    try {
        const int listen_fd = listen_tcp(port, port_file);
        set_nonblocking(listen_fd);
        Server server(workers, queue_cap, listen_fd);
        server.run();
        close(listen_fd);
    } catch (const std::exception &e) {
        std::fprintf(stderr, "server: %s\n", e.what());
        return 1;
    }
    return 0;
}
