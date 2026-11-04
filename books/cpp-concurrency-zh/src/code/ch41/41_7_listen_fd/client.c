// mode: run
// 客户端：连 127.0.0.1:PORT，发一行 ping，等回显。
#define _POSIX_C_SOURCE 200809L

#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

int main(int argc, char **argv)
{
    struct sockaddr_in addr;
    char buf[64] = {0};
    int fd;
    ssize_t n;

    if (argc != 2) {
        fprintf(stderr, "usage: %s PORT\n", argv[0]);
        return 2;
    }

    fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd == -1) {
        perror("socket");
        return 1;
    }
    memset(&addr, 0, sizeof addr);
    addr.sin_family = AF_INET;
    addr.sin_port = htons((unsigned short)atoi(argv[1]));
    addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    if (connect(fd, (struct sockaddr *)&addr, sizeof addr) == -1) {
        perror("connect");
        return 1;
    }

    if (write(fd, "ping", 4) != 4) {
        perror("write");
        return 1;
    }
    printf("client: sent ping\n");
    n = read(fd, buf, sizeof buf - 1);
    if (n <= 0) {
        perror("read");
        return 1;
    }
    printf("client: received %s\n", buf);
    close(fd);
    return 0;
}
