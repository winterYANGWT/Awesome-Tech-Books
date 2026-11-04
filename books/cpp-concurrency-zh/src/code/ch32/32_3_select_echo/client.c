// 读 server.port，发一行 "hello"，等回射后退出。
#define _GNU_SOURCE
#include <arpa/inet.h>
#include <errno.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

int main(int argc, char **argv) {
    if (argc != 2) {
        fprintf(stderr, "usage: %s PORT_FILE\n", argv[0]);
        return 2;
    }
    FILE *f = fopen(argv[1], "r");
    if (f == NULL) { perror("port file"); return 1; }
    unsigned port = 0;
    if (fscanf(f, "%u", &port) != 1) { fclose(f); return 1; }
    fclose(f);

    int fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd < 0) { perror("socket"); return 1; }
    struct sockaddr_in addr;
    memset(&addr, 0, sizeof addr);
    addr.sin_family = AF_INET;
    addr.sin_port = htons((unsigned short)port);
    addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    if (connect(fd, (struct sockaddr *)&addr, sizeof addr) != 0) {
        perror("connect");
        return 1;
    }
    const char msg[] = "hello";
    if (send(fd, msg, sizeof msg - 1, MSG_NOSIGNAL) != (ssize_t)(sizeof msg - 1)) {
        perror("send");
        return 1;
    }
    char buf[16];
    size_t got = 0;
    while (got < sizeof msg - 1) {
        ssize_t n = recv(fd, buf + got, sizeof buf - got, 0);
        if (n <= 0) { perror("recv"); return 1; }
        got += (size_t)n;
    }
    close(fd);
    return 0;
}
