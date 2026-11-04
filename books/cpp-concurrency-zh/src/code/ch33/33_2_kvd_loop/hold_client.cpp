#include "../../kvd/net.hpp"

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

#include <chrono>
#include <cstdio>
#include <cstring>
#include <system_error>
#include <thread>

int main(int argc, char **argv) {
    const char *port_file = nullptr;
    int seconds = 3;
    for (int i = 1; i < argc; ++i) {
        if (std::strcmp(argv[i], "--port-file") == 0 && i + 1 < argc) {
            port_file = argv[++i];
        } else if (std::strcmp(argv[i], "--seconds") == 0 && i + 1 < argc) {
            seconds = std::atoi(argv[++i]);
        }
    }
    if (port_file == nullptr) return 2;

    const auto port = read_port_file(port_file, std::chrono::seconds(5));
    const int fd = ::socket(AF_INET, SOCK_STREAM | SOCK_CLOEXEC, 0);
    if (fd < 0) throw std::system_error(errno, std::generic_category(), "socket");
    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    if (::connect(fd, reinterpret_cast<sockaddr *>(&addr), sizeof addr) < 0)
        throw std::system_error(errno, std::generic_category(), "connect");
    std::printf("hold: connected, 不发请求\n");
    std::this_thread::sleep_for(std::chrono::seconds(seconds));
    char c = 0;
    const ssize_t n = ::recv(fd, &c, 1, 0);
    std::printf("hold: recv=%zd (%s)\n", n, n == 0 ? "server closed" : "unexpected");
    ::close(fd);
    return 0;
}
