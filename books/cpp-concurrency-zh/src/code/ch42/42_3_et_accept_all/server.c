// ET 监听 socket：一次事件里循环 accept 到 EAGAIN，把队列取空。
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
    // ANCHOR: loop
    int accepted = 0;
    for (;;) {
        int cfd = accept(lfd, NULL, NULL);
        if (cfd < 0) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                break;  // 队列取空，ET 下一次边沿才需要再 accept
            }
            if (errno == EINTR) {
                continue;
            }
            die("accept");
        }
        serve(cfd);
        close(cfd);
        accepted++;
    }
    // ANCHOR_END: loop
    printf("accepted=%d (loop until EAGAIN)\n", accepted);

    n = epoll_wait(ep, out, 4, 300);
    printf("next epoll_wait=%d (queue drained)\n", n);
    close(ep);
    close(lfd);
    return 0;
}
