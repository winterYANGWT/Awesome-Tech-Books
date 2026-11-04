// mode: run
// output: varies
// 屏蔽 SIGCHLD，用 signalfd 把信号变成可 epoll 的 fd；收到后仍要 waitpid。
#define _GNU_SOURCE

#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/epoll.h>
#include <sys/signalfd.h>
#include <sys/wait.h>
#include <unistd.h>

int main(void)
{
    sigset_t mask;
    struct epoll_event ev;
    int sfd, epfd, reads = 0, reaped = 0;
    unsigned signo = 0;

    sigemptyset(&mask);
    sigaddset(&mask, SIGCHLD);
    if (sigprocmask(SIG_BLOCK, &mask, NULL) == -1) {
        perror("sigprocmask");
        return 1;
    }

    sfd = signalfd(-1, &mask, SFD_CLOEXEC | SFD_NONBLOCK);
    if (sfd == -1) {
        perror("signalfd");
        return 1;
    }
    epfd = epoll_create1(EPOLL_CLOEXEC);
    if (epfd == -1) {
        perror("epoll_create1");
        return 1;
    }
    memset(&ev, 0, sizeof ev);
    ev.events = EPOLLIN;
    ev.data.fd = sfd;
    if (epoll_ctl(epfd, EPOLL_CTL_ADD, sfd, &ev) == -1) {
        perror("epoll_ctl");
        return 1;
    }

    for (int i = 0; i < 3; i++) {
        pid_t pid = fork();
        if (pid == 0) {
            _exit(0);
        }
        if (pid == -1) {
            perror("fork");
            return 1;
        }
    }

    while (reaped < 3) {
        struct signalfd_siginfo si;
        int n = epoll_wait(epfd, &ev, 1, -1);

        if (n == -1) {
            if (errno == EINTR) {
                continue;
            }
            perror("epoll_wait");
            return 1;
        }
        while (read(sfd, &si, sizeof si) == (ssize_t)sizeof si) {
            reads++;                   /* 多个子进程的 SIGCHLD 可能被合并 */
            signo = si.ssi_signo;
        }
        while (waitpid(-1, NULL, WNOHANG) > 0) {
            reaped++;
        }
    }

    printf("signalfd reads=%d, signo=SIGCHLD: %s\n",
           reads, signo == SIGCHLD ? "yes" : "no");
    printf("reaped=%d\n", reaped);
    close(epfd);
    close(sfd);
    return 0;
}
