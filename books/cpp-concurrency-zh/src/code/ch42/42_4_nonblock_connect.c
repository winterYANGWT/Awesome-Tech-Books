// mode: run
// expect: connect: -1, errno=115
// expect: getsockopt: ret=0, err=111
// 非阻塞 connect 到本机一个刚关闭的端口：可写只表示有结果，结果在 SO_ERROR 里。
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

int main(void) {
    // 先绑一个端口再关掉，保证这个端口上没有监听者。
    int probe = socket(AF_INET, SOCK_STREAM | SOCK_CLOEXEC, 0);
    if (probe < 0) {
        perror("socket");
        return 1;
    }
    struct sockaddr_in addr;
    memset(&addr, 0, sizeof addr);
    addr.sin_family = AF_INET;
    addr.sin_port = 0;
    addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    if (bind(probe, (struct sockaddr *)&addr, sizeof addr) != 0) {
        perror("bind");
        return 1;
    }
    socklen_t len = sizeof addr;
    if (getsockname(probe, (struct sockaddr *)&addr, &len) != 0) {
        perror("getsockname");
        return 1;
    }
    close(probe);

    int fd = socket(AF_INET, SOCK_STREAM | SOCK_CLOEXEC | SOCK_NONBLOCK, 0);
    if (fd < 0) {
        perror("socket");
        return 1;
    }
    errno = 0;
    int ret = connect(fd, (struct sockaddr *)&addr, sizeof addr);
    printf("connect: %d, errno=%d (%s)\n", ret, errno, strerror(errno));

    int ep = epoll_create1(EPOLL_CLOEXEC);
    if (ep < 0) {
        perror("epoll_create1");
        return 1;
    }
    struct epoll_event ev = {.events = EPOLLOUT, .data.fd = fd};
    if (epoll_ctl(ep, EPOLL_CTL_ADD, fd, &ev) != 0) {
        perror("epoll_ctl");
        return 1;
    }
    struct epoll_event out;
    ret = epoll_wait(ep, &out, 1, 2000);
    printf("epoll_wait: %d, EPOLLOUT=%d\n", ret,
           ret > 0 && (out.events & EPOLLOUT) != 0);

    // getsockopt 自己成功时返回 0；连接的错误码放在 err 里。
    int err = 0;
    socklen_t errlen = sizeof err;
    errno = 0;
    ret = getsockopt(fd, SOL_SOCKET, SO_ERROR, &err, &errlen);
    printf("getsockopt: ret=%d, err=%d (%s), errno=%d\n",
           ret, err, strerror(err), errno);
    close(ep);
    close(fd);
    return 0;
}
