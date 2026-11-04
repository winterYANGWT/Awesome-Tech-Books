// 测试客户端：连 N 次，每次发一个字节、读回显、关闭。
#include "net.hpp"

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

#include <cstdio>
#include <cstdlib>
#include <cstring>

int main(int argc, char **argv) {
    const char *port_file = nullptr;
    int count = 20;
    for (int i = 1; i < argc; ++i) {
        if (std::strcmp(argv[i], "--port-file") == 0 && i + 1 < argc) {
            port_file = argv[++i];
        } else if (std::strcmp(argv[i], "--connections") == 0 && i + 1 < argc) {
            count = std::atoi(argv[++i]);
        }
    }
    if (port_file == nullptr) {
        std::fprintf(stderr, "usage: %s --port-file F [--connections N]\n", argv[0]);
        return 2;
    }
    try {
        const std::uint16_t port = read_port_file(port_file, std::chrono::seconds(10));
        for (int i = 0; i < count; ++i) {
            int fd = socket(AF_INET, SOCK_STREAM | SOCK_CLOEXEC, 0);
            if (fd < 0) return 1;
            sockaddr_in addr{};
            addr.sin_family = AF_INET;
            addr.sin_port = htons(port);
            addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
            if (connect(fd, reinterpret_cast<sockaddr *>(&addr), sizeof addr) != 0) {
                close(fd);
                return 1;
            }
            const char byte = 'x';
            if (write(fd, &byte, 1) != 1) {
                close(fd);
                return 1;
            }
            char back = 0;
            if (read(fd, &back, 1) != 1) {
                close(fd);
                return 1;
            }
            close(fd);
        }
        std::printf("client: connections=%d\n", count);
    } catch (const std::exception &e) {
        std::fprintf(stderr, "client: %s\n", e.what());
        return 1;
    }
    return 0;
}
