// 4 个线程的领导者/追随者回射服务器。任意时刻只有一个 Leader 在 epoll_wait；
// 它拿到事件后先把某个 Follower 推成新 Leader，再自己处理这个连接。
// mode: run
// output: varies
// expect: leader/followers: connections=200
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <unistd.h>

#include <atomic>
#include <cerrno>
#include <condition_variable>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <mutex>
#include <thread>
#include <vector>

namespace {

constexpr int kThreads = 4;
constexpr int kConnections = 200;

enum State { kFollower, kLeader, kProcessing };

std::mutex mu;
std::condition_variable cv;
State states[kThreads] = {kLeader, kFollower, kFollower, kFollower};
int leader = 0;
bool stop = false;
int epfd = -1;
int listen_fd = -1;
std::atomic<std::uint64_t> handled[kThreads];

// 必须在持锁时调用：从等待中的 Follower 里挑一个当 Leader。
void promote_new_leader(int current) {
    int next = -1;
    for (int k = 1; k <= kThreads; ++k) {
        const int i = (current + k) % kThreads;
        if (states[i] == kFollower) {
            next = i;
            break;
        }
    }
    if (next >= 0) {
        states[next] = kLeader;
        leader = next;
    } else {
        leader = -1;  // 全都在处理；谁先处理完谁接任
    }
    cv.notify_all();
}

void echo_connection(int fd) {
    char buf[4096];
    for (;;) {
        const ssize_t n = read(fd, buf, sizeof buf);
        if (n > 0) {
            ssize_t sent = 0;
            while (sent < n) {
                const ssize_t w = write(fd, buf + sent, static_cast<std::size_t>(n - sent));
                if (w <= 0) {
                    close(fd);
                    return;
                }
                sent += w;
            }
            continue;
        }
        if (n < 0 && errno == EINTR) continue;
        break;
    }
    close(fd);
}

void worker(int id) {
    for (;;) {
        {
            std::unique_lock<std::mutex> lock(mu);
            cv.wait(lock, [id] { return stop || leader == id; });
            if (stop) return;
            states[id] = kLeader;
        }

        epoll_event event{};
        const int n = epoll_wait(epfd, &event, 1, 100);
        if (n < 0 && errno != EINTR) {
            std::lock_guard<std::mutex> lock(mu);
            states[id] = kFollower;
            promote_new_leader(id);
            continue;
        }
        if (n == 0) {
            std::lock_guard<std::mutex> lock(mu);
            if (stop) return;
            // 没事件就继续当 Leader，不推选。
            continue;
        }

        const int conn = accept4(listen_fd, nullptr, nullptr, SOCK_CLOEXEC);
        if (conn >= 0) {
            {
                std::lock_guard<std::mutex> lock(mu);
                states[id] = kProcessing;
                promote_new_leader(id);  // 先推选，再处理
            }
            handled[id].fetch_add(1, std::memory_order_relaxed);
            echo_connection(conn);
        }

        // 处理完回到 Follower；如果刚才没人可推选，就自己接任 Leader。
        std::unique_lock<std::mutex> lock(mu);
        states[id] = kFollower;
        if (leader < 0) {
            states[id] = kLeader;
            leader = id;
        }
        cv.notify_all();
        cv.wait(lock, [id] { return stop || leader == id; });
        if (stop) return;
    }
}

void client_once(std::uint16_t port) {
    int fd = socket(AF_INET, SOCK_STREAM | SOCK_CLOEXEC, 0);
    if (fd < 0) return;
    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    if (connect(fd, reinterpret_cast<sockaddr *>(&addr), sizeof addr) != 0) {
        close(fd);
        return;
    }
    const char msg[] = "hello";
    const ssize_t sent = write(fd, msg, sizeof msg - 1);
    if (sent != static_cast<ssize_t>(sizeof msg - 1)) {
        close(fd);
        return;
    }
    char buf[16];
    ssize_t got = 0;
    while (got < static_cast<ssize_t>(sizeof msg - 1)) {
        const ssize_t n = read(fd, buf + got, sizeof buf - static_cast<std::size_t>(got));
        if (n <= 0) break;
        got += n;
    }
    close(fd);
}

}  // namespace

int main() {
    listen_fd = socket(AF_INET, SOCK_STREAM | SOCK_CLOEXEC, 0);
    if (listen_fd < 0) return 1;
    int yes = 1;
    setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof yes);
    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = 0;
    addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    if (bind(listen_fd, reinterpret_cast<sockaddr *>(&addr), sizeof addr) != 0) return 1;
    if (listen(listen_fd, 128) != 0) return 1;
    socklen_t len = sizeof addr;
    getsockname(listen_fd, reinterpret_cast<sockaddr *>(&addr), &len);
    const std::uint16_t port = ntohs(addr.sin_port);

    epfd = epoll_create1(EPOLL_CLOEXEC);
    epoll_event ev{};
    ev.events = EPOLLIN;
    ev.data.fd = listen_fd;
    epoll_ctl(epfd, EPOLL_CTL_ADD, listen_fd, &ev);

    std::vector<std::thread> threads;
    for (int i = 0; i < kThreads; ++i) {
        handled[i].store(0);
        threads.emplace_back(worker, i);
    }

    for (int i = 0; i < kConnections; ++i) client_once(port);

    {
        std::lock_guard<std::mutex> lock(mu);
        stop = true;
    }
    cv.notify_all();
    for (auto &t : threads) t.join();

    std::printf("leader/followers: connections=%d\n", kConnections);
    std::uint64_t total = 0;
    for (int i = 0; i < kThreads; ++i) {
        const auto n = handled[i].load();
        total += n;
        std::printf("thread %d: %llu\n", i, static_cast<unsigned long long>(n));
    }
    std::printf("total: %llu\n", static_cast<unsigned long long>(total));
    close(epfd);
    close(listen_fd);
}
