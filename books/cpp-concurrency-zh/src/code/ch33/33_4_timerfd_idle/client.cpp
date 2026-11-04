#include "../../kvd/net.hpp"

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

#include <chrono>
#include <cstdio>
#include <cstring>
#include <string>
#include <thread>

namespace {
int connect_to(std::uint16_t port) {
    const int fd = ::socket(AF_INET, SOCK_STREAM | SOCK_CLOEXEC, 0);
    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    if (::connect(fd, reinterpret_cast<sockaddr *>(&addr), sizeof addr) < 0) {
        ::close(fd);
        return -1;
    }
    return fd;
}
bool read_exact(int fd, char *buf, std::size_t n) {
    std::size_t got = 0;
    while (got < n) {
        const ssize_t r = ::recv(fd, buf + got, n - got, 0);
        if (r > 0) got += static_cast<std::size_t>(r);
        else if (r < 0 && errno == EINTR) continue;
        else return false;
    }
    return true;
}
}  // namespace

int main(int argc, char **argv) {
    const char *port_file = nullptr;
    const char *mode = "active";
    int seconds = 6;
    for (int i = 1; i < argc; ++i) {
        if (std::strcmp(argv[i], "--port-file") == 0 && i + 1 < argc) port_file = argv[++i];
        else if (std::strcmp(argv[i], "--mode") == 0 && i + 1 < argc) mode = argv[++i];
        else if (std::strcmp(argv[i], "--seconds") == 0 && i + 1 < argc) seconds = std::atoi(argv[++i]);
    }
    if (port_file == nullptr) return 2;

    const auto port = read_port_file(port_file, std::chrono::seconds(5));
    const int fd = connect_to(port);
    if (fd < 0) return 1;

    if (std::strcmp(mode, "idle") == 0) {
        const char ping[] = "PING\n";
        write_all(fd, ping);
        char pong[5];
        if (!read_exact(fd, pong, sizeof pong)) return 1;
        std::printf("idle: connected, one request, then sleep %ds\n", seconds);
        std::this_thread::sleep_for(std::chrono::seconds(seconds));
        char c = 0;
        const ssize_t n = ::recv(fd, &c, 1, 0);
        std::printf("idle: recv=%zd (%s)\n", n, n == 0 ? "server closed" : "unexpected");
    } else {
        std::printf("active: connected, request every 500ms for %ds\n", seconds);
        const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(seconds);
        int pongs = 0;
        while (std::chrono::steady_clock::now() < deadline) {
            const char ping[] = "PING\n";
            if (!write_all(fd, ping)) break;
            char pong[5];
            if (!read_exact(fd, pong, sizeof pong)) break;
            ++pongs;
            std::this_thread::sleep_for(std::chrono::milliseconds(500));
        }
        std::printf("active: %d pongs, still open\n", pongs);
    }
    ::close(fd);
    return 0;
}
