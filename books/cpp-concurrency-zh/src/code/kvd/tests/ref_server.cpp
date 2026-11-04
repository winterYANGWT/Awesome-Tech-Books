// 测试用的最简 kvd 服务器：每个连接一个线程。
//
// 它不进正文，只用来把 kvbench 和协议端到端跑起来。用法：
//     ref_server <port> [port-file]
// port 为 0 时由内核选端口，实际端口写进 port-file。
#include "net.hpp"
#include "protocol.hpp"
#include "store.hpp"

#include <cerrno>
#include <csignal>
#include <cstdint>
#include <cstdlib>
#include <exception>
#include <iostream>
#include <stdexcept>
#include <string>
#include <thread>
#include <variant>

namespace {

void serve(int fd, Store* store) {
    Parser parser;
    char buf[4096];
    try {
        for (;;) {
            ssize_t n = ::recv(fd, buf, sizeof buf, 0);
            if (n == 0) break;
            if (n < 0) {
                if (errno == EINTR) continue;
                kvd_detail::throw_errno("recv");
            }
            parser.feed(std::string_view(buf, static_cast<std::size_t>(n)));
            while (auto result = parser.next()) {
                std::string response;
                if (auto* request = std::get_if<Request>(&*result)) {
                    response = format_response(store->apply(*request));
                } else {
                    response = format_response(std::get<ParseError>(*result));
                }
                if (!write_all(fd, response)) break;
            }
        }
    } catch (const std::exception&) {
        // 客户端断开或写失败：关掉这个连接就够了。
    }
    ::close(fd);
}

}  // namespace

int main(int argc, char** argv) {
    if (argc < 2 || argc > 3) {
        std::cerr << "usage: " << argv[0] << " <port> [port-file]\n";
        return 2;
    }
    std::uint16_t port = 0;
    try {
        int value = std::stoi(argv[1]);
        if (value < 0 || value > 65535) throw std::out_of_range("port");
        port = static_cast<std::uint16_t>(value);
    } catch (const std::exception&) {
        std::cerr << "invalid port: " << argv[1] << '\n';
        return 2;
    }

    std::signal(SIGPIPE, SIG_IGN);  // 对端关掉后 write 返回 EPIPE，不要杀进程

    try {
        int listen_fd = listen_tcp(port, argc == 3 ? argv[2] : nullptr);
        Store store;
        for (;;) {
            int fd = ::accept4(listen_fd, nullptr, nullptr, SOCK_CLOEXEC);
            if (fd < 0) {
                if (errno == EINTR) continue;
                kvd_detail::throw_errno("accept4");
            }
            std::thread(serve, fd, &store).detach();
        }
    } catch (const std::exception& e) {
        std::cerr << "ref_server: " << e.what() << '\n';
        return 1;
    }
}
