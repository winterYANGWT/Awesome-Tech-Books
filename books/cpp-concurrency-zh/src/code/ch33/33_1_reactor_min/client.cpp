#include "../../kvd/net.hpp"

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

#include <cerrno>
#include <chrono>
#include <cstdint>
#include <cstdlib>
#include <system_error>
#include <cstdio>
#include <cstring>
#include <string>
#include <thread>

namespace {

int connect_to(std::uint16_t port) {
    const int fd = ::socket(AF_INET, SOCK_STREAM | SOCK_CLOEXEC, 0);
    if (fd < 0) throw std::system_error(errno, std::generic_category(), "socket");
    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    if (::connect(fd, reinterpret_cast<sockaddr *>(&addr), sizeof addr) < 0) {
        ::close(fd);
        throw std::system_error(errno, std::generic_category(), "connect");
    }
    return fd;
}

bool read_exact(int fd, char *buf, std::size_t n) {
    std::size_t got = 0;
    while (got < n) {
        const ssize_t r = ::recv(fd, buf + got, n - got, 0);
        if (r > 0) {
            got += static_cast<std::size_t>(r);
            continue;
        }
        if (r < 0 && errno == EINTR) continue;
        return false;
    }
    return true;
}

}  // namespace

int main(int argc, char **argv) {
    const char *port_file = nullptr;
    const char *mode = "active";
    int seconds = 2;
    for (int i = 1; i < argc; ++i) {
        if (std::strcmp(argv[i], "--port-file") == 0 && i + 1 < argc) {
            port_file = argv[++i];
        } else if (std::strcmp(argv[i], "--mode") == 0 && i + 1 < argc) {
            mode = argv[++i];
        } else if (std::strcmp(argv[i], "--seconds") == 0 && i + 1 < argc) {
            seconds = std::atoi(argv[++i]);
        }
    }
    if (port_file == nullptr) {
        std::fprintf(stderr, "usage: %s --port-file FILE --mode active|idle\n", argv[0]);
        return 2;
    }

    const auto port = read_port_file(port_file, std::chrono::seconds(5));
    const int fd = connect_to(port);
    if (std::strcmp(mode, "idle") == 0) {
        std::printf("idle: connected, sending nothing for %ds\n", seconds);
        std::this_thread::sleep_for(std::chrono::seconds(seconds));
        char c = 0;
        const ssize_t n = ::recv(fd, &c, 1, 0);
        std::printf("idle: recv=%zd (%s)\n", n, n == 0 ? "server closed" : "unexpected");
    } else {
        std::printf("active: connected, sending for %ds\n", seconds);
        const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(seconds);
        int count = 0;
        while (std::chrono::steady_clock::now() < deadline) {
            const char ping[] = "ping\n";
            if (!write_all(fd, ping)) throw std::system_error(EIO, std::generic_category(), "ping");
            char echo[5];
            if (!read_exact(fd, echo, sizeof echo)) break;
            ++count;
            std::this_thread::sleep_for(std::chrono::milliseconds(300));
        }
        std::printf("active: %d echoes\n", count);
    }
    ::close(fd);
    return 0;
}
