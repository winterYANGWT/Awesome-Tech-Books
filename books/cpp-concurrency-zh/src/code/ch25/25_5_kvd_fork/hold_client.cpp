// 保持一条连接不关，让父进程的某个子进程留在 ps 里。
#include "net.hpp"

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <thread>

int main(int argc, char **argv) {
    if (argc != 3) {
        std::fprintf(stderr, "usage: %s PORT_FILE MILLISECONDS\n", argv[0]);
        return 2;
    }
    const auto port =
        read_port_file(argv[1], std::chrono::milliseconds(5000));
    const int millis = std::atoi(argv[2]);

    int fd = socket(AF_INET, SOCK_STREAM | SOCK_CLOEXEC, 0);
    if (fd < 0) {
        perror("socket");
        return 1;
    }
    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    if (connect(fd, reinterpret_cast<sockaddr *>(&addr), sizeof addr) < 0) {
        perror("connect");
        close(fd);
        return 1;
    }

    write_all(fd, "SET k 1\n");
    char buf[128];
    ssize_t n = recv(fd, buf, sizeof buf, 0);
    (void)n;
    std::this_thread::sleep_for(std::chrono::milliseconds(millis));
    close(fd);
    return 0;
}
