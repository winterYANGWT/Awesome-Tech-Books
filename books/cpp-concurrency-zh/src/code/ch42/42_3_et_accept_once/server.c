// ET 监听 socket，但每次事件只 accept 一个连接。
// 两个连接在进入事件循环前就已经排在队列里，所以只有一个边沿。
#define _GNU_SOURCE
#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <unistd.h>

static void die(const char *what) {
    perror(what);
    exit(1);
}

static void write_port(const char *path, int port) {
    FILE *f = fopen(path, "w");
    if (!f) {
        die("fopen port file");
    }
    fprintf(f, "%d\n", port);
    fclose(f);
}

static void wait_for(const char *path) {
    for (int i = 0; i < 5000; i++) {
        if (access(path, F_OK) == 0) {
            return;
        }
        usleep(1000);
    }
    fprintf(stderr, "ready file did not appear\n");
    exit(1);
}

static void serve(int fd) {
    char buf[64];
    ssize_t n;
    do {
        n = read(fd, buf, sizeof buf);
    } while (n < 0 && errno == EINTR);
    if (n <= 0) {
        return;
    }
    const char reply[] = "OK\n";
    if (write(fd, reply, sizeof reply - 1) < 0) {
        perror("write");
    }
}

int main(int argc, char **argv) {
    if (argc != 3) {
        fprintf(stderr, "usage: %s PORT_FILE READY_FILE\n", argv[0]);
        return 2;
    }

    int lfd = socket(AF_INET, SOCK_STREAM | SOCK_CLOEXEC | SOCK_NONBLOCK, 0);
    if (lfd < 0) {
        die("socket");
    }
    int yes = 1;
    if (setsockopt(lfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof yes) != 0) {
        die("SO_REUSEADDR");
    }
    struct sockaddr_in addr;
    memset(&addr, 0, sizeof addr);
    addr.sin_family = AF_INET;
    addr.sin_port = 0;  // 让内核选端口
    addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    if (bind(lfd, (struct sockaddr *)&addr, sizeof addr) != 0) {
        die("bind");
    }
    if (listen(lfd, 16) != 0) {
        die("listen");
    }
    socklen_t len = sizeof addr;
    if (getsockname(lfd, (struct sockaddr *)&addr, &len) != 0) {
        die("getsockname");
    }
    write_port(argv[1], ntohs(addr.sin_port));

    int ep = epoll_create1(EPOLL_CLOEXEC);
    if (ep < 0) {
        die("epoll_create1");
    }
    struct epoll_event ev = {.events = EPOLLIN | EPOLLET, .data.fd = lfd};
    if (epoll_ctl(ep, EPOLL_CTL_ADD, lfd, &ev) != 0) {
        die("epoll_ctl");
    }

    // 等客户端把两条连接都建好，避免它只赶上第一条的边沿。
    wait_for(argv[2]);

    struct epoll_event out[4];
    int n = epoll_wait(ep, out, 4, 2000);
    if (n <= 0) {
        fprintf(stderr, "no event: %d\n", n);
        return 1;
    }
    int cfd = accept(lfd, NULL, NULL);  // 只取队头一个
    if (cfd < 0) {
        die("accept");
    }
    serve(cfd);
    close(cfd);
    printf("accepted=1\n");

    n = epoll_wait(ep, out, 4, 1500);
    printf("second epoll_wait=%d (no new edge)\n", n);
    close(ep);
    close(lfd);
    return 0;
}
