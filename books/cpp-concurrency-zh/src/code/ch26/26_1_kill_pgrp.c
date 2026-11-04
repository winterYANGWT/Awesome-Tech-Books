// mode: run
// expect: 父子在同一进程组：1
// expect: 父进程收到=1 子进程收到=1 合计=2
#define _GNU_SOURCE
#define _POSIX_C_SOURCE 200809L
#include <errno.h>
#include <fcntl.h>
#include <poll.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <unistd.h>

static int data_fd = -1;
static volatile char who = 'P';

static void on_usr1(int signo)
{
    int saved = errno;
    char tag = who;
    (void)signo;
    if (data_fd >= 0) {
        ssize_t n = write(data_fd, &tag, 1);
        (void)n;
    }
    errno = saved;
}

int main(void)
{
    int data[2], ready[2];
    struct sigaction sa = {0};
    sigset_t usr1, empty;
    pid_t child;
    int parent_bytes = 0, child_bytes = 0;

    if (getpgrp() != getpid() && setpgid(0, 0) == -1) {
        perror("setpgid");
        return 1;
    }
    if (pipe2(data, O_NONBLOCK | O_CLOEXEC) == -1 ||
        pipe2(ready, O_CLOEXEC) == -1) {
        perror("pipe2");
        return 1;
    }
    data_fd = data[1];

    sa.sa_handler = on_usr1;
    sigemptyset(&sa.sa_mask);
    if (sigaction(SIGUSR1, &sa, NULL) == -1) {
        perror("sigaction");
        return 1;
    }

    child = fork();
    if (child == -1) {
        perror("fork");
        return 1;
    }
    if (child == 0) {
        char b = 'C';
        close(ready[0]);
        sigemptyset(&usr1);
        sigaddset(&usr1, SIGUSR1);
        if (sigprocmask(SIG_BLOCK, &usr1, NULL) == -1)
            _exit(2);
        who = 'C';
        if (write(ready[1], &b, 1) != 1)
            _exit(3);
        sigemptyset(&empty);
        if (sigsuspend(&empty) == -1 && errno != EINTR)
            _exit(4);
        _exit(0);
    }

    close(ready[1]);
    {
        char b;
        if (read(ready[0], &b, 1) != 1) {
            perror("read ready");
            return 1;
        }
    }
    printf("父子在同一进程组：%d\n", getpgid(child) == getpgrp());

    if (kill(0, SIGUSR1) == -1) {
        perror("kill");
        return 1;
    }

    {
        struct pollfd pfd = {.fd = data[0], .events = POLLIN};
        int total = 0;
        while (total < 2) {
            int n = poll(&pfd, 1, 2000);
            if (n <= 0)
                break;
            char buf[8];
            ssize_t got = read(data[0], buf, sizeof buf);
            if (got <= 0)
                break;
            for (ssize_t i = 0; i < got; ++i) {
                if (buf[i] == 'P')
                    ++parent_bytes;
                else if (buf[i] == 'C')
                    ++child_bytes;
            }
            total += (int)got;
        }
    }

    if (waitpid(child, NULL, 0) == -1) {
        perror("waitpid");
        return 1;
    }
    printf("父进程收到=%d 子进程收到=%d 合计=%d\n",
           parent_bytes, child_bytes, parent_bytes + child_bytes);
    return (parent_bytes == 1 && child_bytes == 1) ? 0 : 1;
}
