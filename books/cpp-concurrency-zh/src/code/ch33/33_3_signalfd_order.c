// mode: run
// tsan: no（信号处理函数与主线程的交接不在 TSan 的模型内）
// 先装 handler、不屏蔽信号：信号走 handler，signalfd 读不到。
#define _GNU_SOURCE

#include <errno.h>
#include <poll.h>
#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <sys/signalfd.h>
#include <unistd.h>

static volatile sig_atomic_t handler_count;

static void on_term(int signo) {
    (void)signo;
    ++handler_count;
}

int main(void) {
    struct sigaction sa;
    memset(&sa, 0, sizeof sa);
    sa.sa_handler = on_term;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    sigaction(SIGTERM, &sa, NULL);  // 先装 handler

    sigset_t mask;
    sigemptyset(&mask);
    sigaddset(&mask, SIGTERM);
    int sfd = signalfd(-1, &mask, SFD_NONBLOCK | SFD_CLOEXEC);  // 没有屏蔽

    kill(getpid(), SIGTERM);
    for (int i = 0; i < 100 && handler_count == 0; ++i) usleep(1000);

    struct pollfd pfd;
    pfd.fd = sfd;
    pfd.events = POLLIN;
    int rc = poll(&pfd, 1, 300);
    if (rc < 0 && errno == EINTR) rc = poll(&pfd, 1, 300);

    printf("handler count=%d\n", (int)handler_count);
    if (rc == 0) printf("signalfd: timeout（信号没有进 fd）\n");
    else if (rc > 0) printf("signalfd: readable\n");
    else printf("poll error\n");
    close(sfd);
    return 0;
}
