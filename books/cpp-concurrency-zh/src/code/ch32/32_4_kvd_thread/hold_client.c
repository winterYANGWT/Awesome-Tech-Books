// 连上服务器后什么都不发，保持连接 ms 毫秒，模拟一个慢客户端。
#define _GNU_SOURCE
#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

int main(int argc, char **argv) {
    if (argc != 3) {
        fprintf(stderr, "usage: hold_client PORT_FILE MS\n");
        return 2;
    }
    FILE *f = fopen(argv[1], "r");
    if (f == NULL) { perror("port file"); return 1; }
    unsigned port = 0;
    if (fscanf(f, "%u", &port) != 1) { fclose(f); return 1; }
    fclose(f);
    int ms = atoi(argv[2]);

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
    usleep((useconds_t)ms * 1000);
    close(fd);
    return 0;
}
