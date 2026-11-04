#include "../../kvd/net.hpp"

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

#include <cerrno>
#include <chrono>
#include <cstdio>
#include <cstring>
#include <string>

int main(int argc, char **argv) {
    const char *port_file = nullptr;
    for (int i = 1; i < argc; ++i)
        if (std::strcmp(argv[i], "--port-file") == 0 && i + 1 < argc) port_file = argv[++i];
    if (port_file == nullptr) return 2;

    const auto port = read_port_file(port_file, std::chrono::seconds(5));
    const int fd = ::socket(AF_INET, SOCK_STREAM | SOCK_CLOEXEC, 0);
    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    if (::connect(fd, reinterpret_cast<sockaddr *>(&addr), sizeof addr) < 0) return 1;

    const char request[] = "GET v\n";
    write_all(fd, request);
    std::printf("client: sent GET v\n");

    std::string line;
    char c = 0;
    while (::recv(fd, &c, 1, 0) == 1) {
        line.push_back(c);
        if (c == '\n') break;
    }
    std::printf("client: %s", line.c_str());

    const ssize_t n = ::recv(fd, &c, 1, 0);
    std::printf("client: recv=%zd (%s)\n", n, n == 0 ? "server closed after full response" : "unexpected");
    ::close(fd);
    return line == "VALUE hello\n" ? 0 : 1;
}
