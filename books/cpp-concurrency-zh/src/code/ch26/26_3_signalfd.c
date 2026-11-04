// mode: run
// expect: timerfd：定时到期
// expect: eventfd：收到唤醒
// expect: signalfd：收到 SIGTERM
#define _GNU_SOURCE
#define _POSIX_C_SOURCE 200809L
#include <errno.h>
#include <pthread.h>
#include <signal.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sys/epoll.h>
#include <sys/eventfd.h>
#include <sys/signalfd.h>
#include <sys/timerfd.h>
#include <time.h>
#include <unistd.h>

static void sleep_ms(long ms)
{
    struct timespec ts = {.tv_sec = ms / 1000,
                          .tv_nsec = (ms % 1000) * 1000000L};
    while (nanosleep(&ts, &ts) == -1 && errno == EINTR)
        ;
}

struct helper_arg {
    int efd;
};

static void *helper(void *arg)
{
    struct helper_arg *a = arg;
    uint64_t one = 1;
    sleep_ms(100);
    ssize_t n = write(a->efd, &one, sizeof one);
    (void)n;
    sleep_ms(50);
    (void)kill(getpid(), SIGTERM);
    return NULL;
}

int main(void)
{
    sigset_t mask;
    int sfd, tfd, efd, epfd;
    pthread_t th;
    struct helper_arg arg;

    /* signalfd 只接管被屏蔽的信号；这里先屏蔽，再创建线程。 */
    sigemptyset(&mask);
    sigaddset(&mask, SIGTERM);
    if (pthread_sigmask(SIG_BLOCK, &mask, NULL) != 0)
        return 1;

    sfd = signalfd(-1, &mask, SFD_NONBLOCK | SFD_CLOEXEC);
    tfd = timerfd_create(CLOCK_MONOTONIC, TFD_NONBLOCK | TFD_CLOEXEC);
    efd = eventfd(0, EFD_NONBLOCK | EFD_CLOEXEC);
    epfd = epoll_create1(EPOLL_CLOEXEC);
    if (sfd == -1 || tfd == -1 || efd == -1 || epfd == -1)
        return 1;

    struct itimerspec its = {.it_value = {.tv_sec = 0, .tv_nsec = 30000000}};
    if (timerfd_settime(tfd, 0, &its, NULL) == -1)
        return 1;

    struct epoll_event events[3];
    events[0].events = EPOLLIN;
    events[0].data.fd = tfd;
    epoll_ctl(epfd, EPOLL_CTL_ADD, tfd, &events[0]);
    events[0].data.fd = efd;
    epoll_ctl(epfd, EPOLL_CTL_ADD, efd, &events[0]);
    events[0].data.fd = sfd;
    epoll_ctl(epfd, EPOLL_CTL_ADD, sfd, &events[0]);

    arg.efd = efd;
    if (pthread_create(&th, NULL, helper, &arg) != 0)
        return 1;

    int seen = 0;
    while (seen < 3) {
        int n = epoll_wait(epfd, events, 3, 2000);
        if (n == -1) {
            if (errno == EINTR)
                continue;
            break;
        }
        for (int i = 0; i < n; ++i) {
            int fd = events[i].data.fd;
            if (fd == tfd) {
                uint64_t exp;
                if (read(tfd, &exp, sizeof exp) == (ssize_t)sizeof exp)
                    printf("timerfd：定时到期\n");
                ++seen;
            } else if (fd == efd) {
                uint64_t val;
                if (read(efd, &val, sizeof val) == (ssize_t)sizeof val)
                    printf("eventfd：收到唤醒\n");
                ++seen;
            } else if (fd == sfd) {
                struct signalfd_siginfo si;
                if (read(sfd, &si, sizeof si) == (ssize_t)sizeof si)
                    printf("signalfd：收到 %s\n",
                           si.ssi_signo == SIGTERM ? "SIGTERM" : "其他信号");
                ++seen;
            }
        }
    }

    pthread_join(th, NULL);
    printf("三路 fd 都在同一个 epoll 里\n");
    close(epfd);
    close(sfd);
    close(tfd);
    close(efd);
    return 0;
}
