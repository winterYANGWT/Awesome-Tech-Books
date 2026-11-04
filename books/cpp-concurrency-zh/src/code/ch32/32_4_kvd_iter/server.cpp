// 迭代版 kvd：一次只服务一个连接，处理完（对端关闭）才 accept 下一个。
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
    sa.sa_flags = 0;  // 不重启 accept，SIGTERM 能把它打断
    sigaction(SIGTERM, &sa, nullptr);
    sigaction(SIGINT, &sa, nullptr);
    signal(SIGPIPE, SIG_IGN);

    const int lfd = listen_tcp(port, port_file);
    Store store(16);
    std::uint64_t served = 0;

// ANCHOR: serve
    while (!g_stop) {
        int cfd = accept(lfd, nullptr, nullptr);
        if (cfd < 0) {
            if (errno == EINTR) continue;
            perror("accept");
            break;
        }
        ++served;
        Parser parser;
        char buf[4096];
        for (;;) {
            ssize_t n = recv(cfd, buf, sizeof buf, 0);
            if (n < 0) {
                if (errno == EINTR) continue;
                perror("recv");
                break;
            }
            if (n == 0) break;
            parser.feed(std::string_view(buf, static_cast<std::size_t>(n)));
            while (auto result = parser.next()) {
                std::string out;
                if (auto *req = std::get_if<Request>(&*result)) {
                    out = format_response(store.apply(*req));
                } else {
                    out = format_response(std::get<ParseError>(*result));
                }
                try {
                    if (!write_all(cfd, out)) break;
                } catch (...) {
                    // 对端已经关闭；这条连接到此为止。
                    break;
                }
            }
        }
        close(cfd);
    }

// ANCHOR_END: serve
    close(lfd);
    std::printf("server: served=%llu\n", (unsigned long long)served);
    return 0;
}
