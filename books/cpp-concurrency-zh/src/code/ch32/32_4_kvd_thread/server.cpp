// 每连接一个 jthread：一个连接阻塞在 recv 上，不影响其他连接被服务。
#include "net.hpp"
#include "protocol.hpp"
#include "store.hpp"

#include <signal.h>
#include <sys/socket.h>
#include <unistd.h>

#include <cerrno>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <string_view>
#include <thread>
#include <vector>

namespace {
volatile sig_atomic_t g_stop = 0;
int g_lfd = -1;
void on_term(int) {
    g_stop = 1;
    // 关掉监听 fd，阻塞在 accept 上的主线程才会醒；close 是 async-signal-safe 的。
    if (g_lfd >= 0) {
        close(g_lfd);
        g_lfd = -1;
    }
}

std::uint16_t parse_port(const char *text) {
    char *end = nullptr;
    unsigned long v = std::strtoul(text, &end, 10);
    if (end == text || *end != '\0' || v > 65535) {
        std::fprintf(stderr, "bad port: %s\n", text);
        std::exit(2);
    }
    return static_cast<std::uint16_t>(v);
}

void serve(int cfd, Store *store) {
    Parser parser;
    char buf[4096];
    for (;;) {
        ssize_t n = recv(cfd, buf, sizeof buf, 0);
        if (n < 0) {
            if (errno == EINTR) continue;
            break;
        }
        if (n == 0) break;
        parser.feed(std::string_view(buf, static_cast<std::size_t>(n)));
        bool failed = false;
        while (auto result = parser.next()) {
            std::string out;
            if (auto *req = std::get_if<Request>(&*result)) {
                out = format_response(store->apply(*req));
            } else {
                out = format_response(std::get<ParseError>(*result));
            }
            try {
                if (!write_all(cfd, out)) {
                    failed = true;
                    break;
                }
            } catch (...) {
                failed = true;
                break;
            }
        }
        if (failed) break;
    }
    close(cfd);
}
}  // namespace

int main(int argc, char **argv) {
    std::uint16_t port = 0;
    const char *port_file = nullptr;
    for (int i = 1; i < argc; ++i) {
        if (std::strcmp(argv[i], "--port") == 0 && i + 1 < argc) {
            port = parse_port(argv[++i]);
        } else if (std::strcmp(argv[i], "--port-file") == 0 && i + 1 < argc) {
            port_file = argv[++i];
        }
    }
    if (port == 0 && port_file == nullptr) {
        std::fprintf(stderr, "usage: %s (--port P | --port-file F)\n", argv[0]);
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

    g_lfd = listen_tcp(port, port_file);
    Store store(16);
    std::vector<std::jthread> workers;
    std::uint64_t served = 0;

// ANCHOR: spawn
    while (!g_stop) {
        int cfd = accept(g_lfd, nullptr, nullptr);
        if (cfd < 0) {
            if (errno == EINTR) continue;
            perror("accept");
            break;
        }
        ++served;
        workers.emplace_back(serve, cfd, &store);
    }
// ANCHOR_END: spawn
    if (g_lfd >= 0) close(g_lfd);
    for (auto &t : workers) t.join();

    std::printf("server: connections=%llu\n", (unsigned long long)served);
    return 0;
}
