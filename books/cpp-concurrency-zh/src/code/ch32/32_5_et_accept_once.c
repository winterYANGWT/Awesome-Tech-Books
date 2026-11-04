// mode: crash
// expect: Assertion
// 监听 fd 注册 ET，但只 accept 一次：队列里剩下的连接不会让 epoll_wait 再报。
#define _GNU_SOURCE
#include <arpa/inet.h>
#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <unistd.h>

static int make_listener(unsigned short *port) {
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd < 0) { perror("socket"); exit(1); }
    int yes = 1;
    setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof yes);
    struct sockaddr_in addr;
    memset(&addr, 0, sizeof addr);
    addr.sin_family = AF_INET;
    addr.sin_port = 0;
    addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    if (bind(fd, (struct sockaddr *)&addr, sizeof addr) != 0) { perror("bind"); exit(1); }
    if (listen(fd, 16) != 0) { perror("listen"); exit(1); }
    socklen_t len = sizeof addr;
    getsockname(fd, (struct sockaddr *)&addr, &len);
    *port = ntohs(addr.sin_port);
    int flags = fcntl(fd, F_GETFL, 0);
    fcntl(fd, F_SETFL, flags | O_NONBLOCK);
    return fd;
}

static void child_connect(unsigned short port, int notify) {
    for (int i = 0; i < 3; ++i) {
        int fd = socket(AF_INET, SOCK_STREAM, 0);
        struct sockaddr_in addr;
        memset(&addr, 0, sizeof addr);
        addr.sin_family = AF_INET;
        addr.sin_port = htons(port);
        addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
        if (connect(fd, (struct sockaddr *)&addr, sizeof addr) != 0) {
            perror("connect");
            _exit(1);
        }
    }
    char c = 'x';
    if (write(notify, &c, 1) != 1) _exit(1);
    usleep(500 * 1000);
    _exit(0);
}

int main(void) {
    unsigned short port = 0;
    int lfd = make_listener(&port);
    int pipefd[2];
    if (pipe(pipefd) != 0) { perror("pipe"); return 1; }

    pid_t pid = fork();
    if (pid == 0) {
        close(pipefd[0]);
        child_connect(port, pipefd[1]);
    }
    close(pipefd[1]);

    // 等三个连接都完成握手，全部排在 backlog 里。
    char c;
    if (read(pipefd[0], &c, 1) != 1) { perror("read"); return 1; }

// ANCHOR: once
    int ep = epoll_create1(EPOLL_CLOEXEC);
    struct epoll_event ev;
    memset(&ev, 0, sizeof ev);
    ev.events = EPOLLIN | EPOLLET;
    ev.data.fd = lfd;
    if (epoll_ctl(ep, EPOLL_CTL_ADD, lfd, &ev) != 0) { perror("epoll_ctl"); return 1; }

    struct epoll_event out[4];
    int n = epoll_wait(ep, out, 4, 1000);
    printf("第一次 epoll_wait 返回 %d\n", n);
    int c1 = accept(lfd, NULL, NULL);
    printf("第一次 accept 返回 %d\n", c1);

    n = epoll_wait(ep, out, 4, 200);
    printf("第二次 epoll_wait 返回 %d（ET 没有新边沿）\n", n);

    errno = 0;
    int c2 = accept(lfd, NULL, NULL);
    printf("再 accept 仍能拿到连接: %d\n", c2);

    // 断言“已经 accept 完”是错的：队列里还有连接。
    assert(c2 < 0 && errno == EAGAIN);

// ANCHOR_END: once
    close(c1);
    if (c2 >= 0) close(c2);
    close(lfd);
    close(ep);
    kill(pid, SIGKILL);
    waitpid(pid, NULL, 0);
    return 0;
}
