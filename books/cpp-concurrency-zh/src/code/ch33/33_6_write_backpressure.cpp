// mode: crash
// expect: Assertion
// 客户端只发不读，服务器把响应全塞进无界输出缓冲，到 8 MiB 断言失败。
#include <poll.h>
#include <sys/socket.h>
#include <unistd.h>

#include <cassert>
#include <cerrno>
#include <cstdio>
#include <cstring>
#include <string>
#include <thread>

namespace {
constexpr std::size_t kLimit = 8 * 1024 * 1024;

void send_request(int fd) {
    const char req[] = "GET k\n";
    for (int i = 0; i < 500000; ++i) {
        const ssize_t n = send(fd, req, 6, MSG_NOSIGNAL);
        if (n == 6) continue;
        if (n < 0 && errno == EAGAIN) {
            pollfd p{fd, POLLOUT, 0};
            poll(&p, 1, 1000);
            --i;
            continue;
        }
        return;
    }
}
}  // namespace

int main() {
    std::setvbuf(stdout, nullptr, _IOLBF, 0);
    int sv[2];
    if (socketpair(AF_UNIX, SOCK_STREAM | SOCK_NONBLOCK | SOCK_CLOEXEC, 0, sv) != 0) return 1;
    int small = 4096;
    setsockopt(sv[0], SOL_SOCKET, SO_SNDBUF, &small, sizeof small);

    std::thread client(send_request, sv[1]);
    client.detach();  // 这个反例只看服务器；进程 abort 时线程一起结束

    const std::string response = "VALUE " + std::string(56, 'x') + "\n";
    std::string out;
    char buf[4096];
    std::uint64_t requests = 0;
    std::size_t printed = 0;

    for (;;) {
        const ssize_t n = recv(sv[0], buf, sizeof buf, 0);
        if (n > 0) {
            const std::size_t count = static_cast<std::size_t>(n) / 6;
            requests += count;
            // ANCHOR: unbounded
            for (std::size_t i = 0; i < count; ++i) out.append(response);  // 无上限
            // ANCHOR_END: unbounded
        } else if (n < 0 && errno == EAGAIN) {
            pollfd p{sv[0], POLLIN, 0};
            poll(&p, 1, 100);
        }

        while (!out.empty()) {
            const ssize_t s = send(sv[0], out.data(), out.size(), MSG_DONTWAIT | MSG_NOSIGNAL);
            if (s > 0) out.erase(0, static_cast<std::size_t>(s));
            else break;
        }
        if (out.size() / (1024 * 1024) > printed) {
            printed = out.size() / (1024 * 1024);
            std::printf("server: out=%zu MiB requests=%llu\n", printed,
                        static_cast<unsigned long long>(requests));
        }
        assert(out.size() <= kLimit);
    }
}
