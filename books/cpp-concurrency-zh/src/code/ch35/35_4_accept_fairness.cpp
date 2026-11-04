// 4 个线程 accept：共享 listen fd + EPOLLEXCLUSIVE，对比各自 SO_REUSEPORT。
// 输出 CSV：第一列是方式，后面 4 列是每个 worker accept 到的连接数。
// mode: run
// output: varies
// expect: mode,w0,w1,w2,w3
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <unistd.h>

#include <atomic>
#include <cerrno>
#include <chrono>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <thread>
#include <vector>

namespace {

constexpr int kWorkers = 4;
constexpr int kConnections = 1200;

int make_listener(std::uint16_t port, std::uint16_t *actual, bool reuseport) {
    int fd = socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK | SOCK_CLOEXEC, 0);
    if (fd < 0) return -1;
    int yes = 1;
    setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof yes);
    if (reuseport) setsockopt(fd, SOL_SOCKET, SO_REUSEPORT, &yes, sizeof yes);

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    if (bind(fd, reinterpret_cast<sockaddr *>(&addr), sizeof addr) < 0) return -1;
    if (listen(fd, 512) < 0) return -1;
    socklen_t len = sizeof addr;
    if (getsockname(fd, reinterpret_cast<sockaddr *>(&addr), &len) < 0) return -1;
    *actual = ntohs(addr.sin_port);
    return fd;
}

void connect_once(std::uint16_t port) {
    int fd = socket(AF_INET, SOCK_STREAM | SOCK_CLOEXEC, 0);
    if (fd < 0) return;
    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    (void)connect(fd, reinterpret_cast<sockaddr *>(&addr), sizeof addr);
    close(fd);
}

struct Counters {
    std::atomic<int> accepted{0};
    std::atomic<bool> stop{false};
    std::atomic<int> per_worker[kWorkers];
    Counters() {
        for (auto &c : per_worker) c.store(0);
    }
};

// 每个线程一个 epoll；listener 由调用者给，可以是共享的，也可以是自己的。
void accept_worker(int index, int listener, int epoll_flags, Counters *counters) {
    int ep = epoll_create1(EPOLL_CLOEXEC);
    epoll_event ev{};
    ev.events = EPOLLIN | static_cast<std::uint32_t>(epoll_flags);
    ev.data.fd = listener;
    if (epoll_ctl(ep, EPOLL_CTL_ADD, listener, &ev) != 0) {
        close(ep);
        return;
    }
    epoll_event events[8];
    while (!counters->stop.load(std::memory_order_relaxed)) {
        const int n = epoll_wait(ep, events, 8, 20);
        if (n <= 0) continue;
        for (int i = 0; i < n; ++i) {
            // 一次事件只 accept 一个连接，这样看到的是唤醒分配，不是某个线程的手速。
            int fd = accept4(listener, nullptr, nullptr, SOCK_NONBLOCK | SOCK_CLOEXEC);
            if (fd >= 0) {
                counters->per_worker[index].fetch_add(1, std::memory_order_relaxed);
                counters->accepted.fetch_add(1, std::memory_order_relaxed);
                close(fd);
            } else if (errno == EINTR || errno == EAGAIN || errno == ECONNABORTED) {
                // EAGAIN 表示别人先 accept 了；不是错误。
            }
        }
    }
    close(ep);
}

void run_case(const char *name, bool reuseport) {
    std::uint16_t port = 0;
    std::vector<int> listeners;
    const int count = reuseport ? kWorkers : 1;
    for (int i = 0; i < count; ++i) {
        const std::uint16_t bind_port = (i == 0) ? 0 : port;
        int fd = make_listener(bind_port, &port, reuseport);
        if (fd < 0) {
            std::perror("make_listener");
            for (int old : listeners) close(old);
            return;
        }
        listeners.push_back(fd);
    }

    Counters counters;
    std::vector<std::thread> threads;
    for (int i = 0; i < kWorkers; ++i) {
        const int listener = reuseport ? listeners[i] : listeners[0];
        const int flags = reuseport ? 0 : static_cast<int>(EPOLLEXCLUSIVE);
        threads.emplace_back(accept_worker, i, listener, flags, &counters);
    }

    // 客户端一个一个连、连上就关；连接在 backlog 里排队，等 accept 拿走。
    for (int i = 0; i < kConnections; ++i) {
        connect_once(port);
    }

    // 等 accept 追上来，避免把还没取走的连接算漏。
    const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(10);
    while (counters.accepted.load() < kConnections &&
           std::chrono::steady_clock::now() < deadline) {
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }

    counters.stop.store(true);
    for (auto &t : threads) t.join();
    for (int fd : listeners) close(fd);

    std::printf("%s", name);
    for (int i = 0; i < kWorkers; ++i) {
        std::printf(",%d", counters.per_worker[i].load());
    }
    std::printf("\n");
}

}  // namespace

int main() {
    std::printf("mode,w0,w1,w2,w3\n");
    run_case("exclusive", false);
    run_case("reuseport", true);
}
