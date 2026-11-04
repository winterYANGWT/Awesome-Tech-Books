// mode: run
// 同一份 100 字节数据：LT 每次都报告，ET 只在状态变化时报告一次。
#define _GNU_SOURCE
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <unistd.h>

static void make_pair(int sv[2]) {
    if (socketpair(AF_UNIX, SOCK_STREAM, 0, sv) != 0) {
        perror("socketpair");
        _exit(1);
    }
}

int main(void) {
    char payload[100];
    memset(payload, 'x', sizeof payload);

    int lt_waits = 0;
    int sv[2];
    make_pair(sv);
    int ep = epoll_create1(EPOLL_CLOEXEC);
    struct epoll_event ev;
    memset(&ev, 0, sizeof ev);
    ev.events = EPOLLIN;
    ev.data.fd = sv[0];
    epoll_ctl(ep, EPOLL_CTL_ADD, sv[0], &ev);
    if (write(sv[1], payload, sizeof payload) != (ssize_t)sizeof payload) return 1;

    char buf[10];
    for (;;) {
        struct epoll_event out[1];
        int n = epoll_wait(ep, out, 1, 1000);
        if (n <= 0) break;
        ++lt_waits;
        ssize_t k = read(sv[0], buf, sizeof buf);
        if (k <= 0) break;
    }
    printf("LT: epoll_wait 报告可读 %d 次\n", lt_waits);
    close(ep);
    close(sv[0]);
    close(sv[1]);

    int et_waits = 0;
    make_pair(sv);
    int flags = fcntl(sv[0], F_GETFL, 0);
    fcntl(sv[0], F_SETFL, flags | O_NONBLOCK);
    ep = epoll_create1(EPOLL_CLOEXEC);
    ev.events = EPOLLIN | EPOLLET;
    ev.data.fd = sv[0];
    epoll_ctl(ep, EPOLL_CTL_ADD, sv[0], &ev);
    if (write(sv[1], payload, sizeof payload) != (ssize_t)sizeof payload) return 1;

    struct epoll_event out[1];
    int n = epoll_wait(ep, out, 1, 1000);
    if (n > 0) {
        ++et_waits;
        ssize_t k = read(sv[0], buf, sizeof buf);  // 只读 10 字节就停
        (void)k;
    }
    n = epoll_wait(ep, out, 1, 100);  // 还剩 90 字节，ET 不再报告
    if (n > 0) ++et_waits;
    printf("ET: epoll_wait 报告可读 %d 次（读了 10 字节后停）\n", et_waits);

    close(ep);
    close(sv[0]);
    close(sv[1]);
    return 0;
}
