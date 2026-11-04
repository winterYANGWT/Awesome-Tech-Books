// mode: run
// 不设 SA_RESTART 时 read 返回 EINTR；设了以后同一个 read 自动重启。
#define _POSIX_C_SOURCE 200809L

#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

static int pipefd[2];

static void on_alarm(int signo)
{
    char byte = 'A';
    ssize_t n;

    (void)signo;
    n = write(pipefd[1], &byte, 1);    /* write 是 async-signal-safe 的 */
    (void)n;
}

static int install(int restart)
{
    struct sigaction sa;

    memset(&sa, 0, sizeof sa);
    sa.sa_handler = on_alarm;
    sa.sa_flags = restart ? SA_RESTART : 0;
    sigemptyset(&sa.sa_mask);
    return sigaction(SIGALRM, &sa, NULL);
}

static void drain_one(void)
{
    char buf[1];

    if (read(pipefd[0], buf, 1) != 1) {
        perror("drain");
    }
}

int main(void)
{
    char byte = '?';
    ssize_t n;

    if (pipe(pipefd) == -1) {
        perror("pipe");
        return 1;
    }
    if (install(0) == -1) {
        perror("sigaction");
        return 1;
    }

    alarm(1);
    n = read(pipefd[0], &byte, 1);
    printf("without SA_RESTART: read returned %zd, EINTR: %s\n",
           n, (n == -1 && errno == EINTR) ? "yes" : "no");
    alarm(0);
    drain_one();

    if (install(1) == -1) {
        perror("sigaction");
        return 1;
    }
    alarm(1);
    n = read(pipefd[0], &byte, 1);
    printf("with SA_RESTART: read returned %zd, byte: %c\n",
           n, n == 1 ? byte : '?');
    return 0;
}
