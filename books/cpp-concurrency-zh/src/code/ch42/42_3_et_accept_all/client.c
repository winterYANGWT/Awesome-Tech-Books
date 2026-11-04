// 两个客户端连接，收到答复打印 OK，1 秒内没有答复就打印超时。
#define _GNU_SOURCE
#include <arpa/inet.h>
#include <errno.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <unistd.h>

static int connect_to(int port) {
    int fd = socket(AF_INET, SOCK_STREAM | SOCK_CLOEXEC, 0);
    if (fd < 0) {
        perror("socket");
        exit(1);
    }
    struct sockaddr_in addr;
    memset(&addr, 0, sizeof addr);
    addr.sin_family = AF_INET;
    addr.sin_port = htons((unsigned short)port);
    addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    if (connect(fd, (struct sockaddr *)&addr, sizeof addr) != 0) {
        perror("connect");
        exit(1);
    }
    struct timeval tv = {.tv_sec = 1, .tv_usec = 0};
    if (setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof tv) != 0) {
        perror("SO_RCVTIMEO");
        exit(1);
    }
    return fd;
}

static void request(int fd) {
    if (write(fd, "PING\n", 5) != 5) {
        perror("write");
        exit(1);
    }
}

static int get_reply(int fd) {
    char buf[64];
    ssize_t n = recv(fd, buf, sizeof buf, 0);
    return n > 0;
}

int main(int argc, char **argv) {
    if (argc != 3) {
        fprintf(stderr, "usage: %s PORT_FILE READY_FILE\n", argv[0]);
        return 2;
    }
    FILE *f = fopen(argv[1], "r");
    if (!f) {
        perror("fopen port file");
        return 1;
    }
    int port = 0;
    if (fscanf(f, "%d", &port) != 1) {
        return 1;
    }
    fclose(f);

    int a = connect_to(port);
    int b = connect_to(port);
    // 两条连接都已建立，通知服务器可以开始 accept 了。
    FILE *ready = fopen(argv[2], "w");
    if (!ready) {
        perror("fopen ready file");
        return 1;
    }
    fclose(ready);

    request(a);
    request(b);
    printf("first: %s\n", get_reply(a) ? "OK" : "timed out");
    if (get_reply(b)) {
        printf("second: OK\n");
    } else {
        printf("second connection timed out\n");
    }
    close(a);
    close(b);
    return 0;
}
