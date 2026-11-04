// mode: run
// 说明 epoll_create 的 size 从 2.6.8 起被忽略，新代码用 epoll_create1(EPOLL_CLOEXEC)。
#define _GNU_SOURCE
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <sys/epoll.h>
#include <unistd.h>

static void show(const char *what, int fd, int err) {
    if (fd < 0) {
        printf("%-32s -> -1, errno=%d (%s)\n", what, err, strerror(err));
        return;
    }
    int flags = fcntl(fd, F_GETFD);
    printf("%-32s -> fd=%d, FD_CLOEXEC=%d\n", what, fd,
           (flags & FD_CLOEXEC) ? 1 : 0);
    close(fd);
}

int main(void) {
    errno = 0;
    int fd = epoll_create(0);
    show("epoll_create(0)", fd, errno);

    errno = 0;
    fd = epoll_create(1);
    show("epoll_create(1)", fd, errno);

    errno = 0;
    fd = epoll_create1(EPOLL_CLOEXEC);
    show("epoll_create1(EPOLL_CLOEXEC)", fd, errno);

    errno = 0;
    fd = epoll_create1(0);
    show("epoll_create1(0)", fd, errno);
    return 0;
}
