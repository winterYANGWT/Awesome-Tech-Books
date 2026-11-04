// mode: run
// 同一个空 socketpair：阻塞 read 被信号打断，非阻塞 read 立即返回 EAGAIN。
#define _GNU_SOURCE
#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

static void on_alarm(int sig) { (void)sig; }

int main(void) {
    int sv[2];
    if (socketpair(AF_UNIX, SOCK_STREAM, 0, sv) != 0) {
        perror("socketpair");
        return 1;
    }

    // 注意用 sigaction 且不设 SA_RESTART；glibc 的 signal() 默认会重启系统调用。
    struct sigaction sa;
    memset(&sa, 0, sizeof sa);
    sa.sa_handler = on_alarm;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    if (sigaction(SIGALRM, &sa, NULL) != 0) {
        perror("sigaction");
        return 1;
    }

    char buf[8];
    alarm(1);
    errno = 0;
    ssize_t n = read(sv[0], buf, sizeof buf);
    printf("阻塞 read 返回 %zd，errno=%s\n", n,
           n < 0 ? strerror(errno) : "0");
    alarm(0);

    int flags = fcntl(sv[0], F_GETFL, 0);
    if (flags < 0 || fcntl(sv[0], F_SETFL, flags | O_NONBLOCK) < 0) {
        perror("fcntl");
        return 1;
    }
    errno = 0;
    n = read(sv[0], buf, sizeof buf);
    printf("非阻塞 read 返回 %zd，errno=%s\n", n,
           n < 0 ? strerror(errno) : "0");

    close(sv[0]);
    close(sv[1]);
    return 0;
}
