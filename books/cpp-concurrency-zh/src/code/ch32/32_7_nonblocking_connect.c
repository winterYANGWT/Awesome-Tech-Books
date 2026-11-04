// mode: run
// 非阻塞 connect 返回 EINPROGRESS；可写只说明有结果，SO_ERROR 才说明结果是什么。
#define _GNU_SOURCE
#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <stdio.h>
#include <string.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <unistd.h>

int main(void) {
    // 先占一个临时端口再关掉，保证这个端口上确实没有监听者。
    int probe = socket(AF_INET, SOCK_STREAM | SOCK_CLOEXEC, 0);
    if (probe < 0) { perror("socket"); return 1; }
    struct sockaddr_in addr;
    memset(&addr, 0, sizeof addr);
    addr.sin_family = AF_INET;
    addr.sin_port = 0;
    addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    if (bind(probe, (struct sockaddr *)&addr, sizeof addr) != 0) { perror("bind"); return 1; }
    socklen_t addrlen = sizeof addr;
    if (getsockname(probe, (struct sockaddr *)&addr, &addrlen) != 0) { perror("getsockname"); return 1; }
    close(probe);

    int fd = socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK | SOCK_CLOEXEC, 0);
    if (fd < 0) { perror("socket"); return 1; }

    errno = 0;
    int rc = connect(fd, (struct sockaddr *)&addr, sizeof addr);
    printf("connect=%d errno=%s\n", rc, strerror(errno));

    int ep = epoll_create1(EPOLL_CLOEXEC);
    struct epoll_event ev;
    memset(&ev, 0, sizeof ev);
    ev.events = EPOLLOUT;
    ev.data.fd = fd;
    epoll_ctl(ep, EPOLL_CTL_ADD, fd, &ev);

    struct epoll_event out[1];
    int n = epoll_wait(ep, out, 1, 2000);
    printf("epoll_wait 返回 %d，可写=%d\n", n,
           n > 0 ? (out[0].events & EPOLLOUT) != 0 : 0);

    int err = 0;
    socklen_t len = sizeof err;
    rc = getsockopt(fd, SOL_SOCKET, SO_ERROR, &err, &len);
    printf("getsockopt=%d SO_ERROR=%s\n", rc, strerror(err));

    close(ep);
    close(fd);
    return 0;
}
