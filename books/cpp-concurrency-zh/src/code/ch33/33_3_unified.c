// mode: run
// signalfd、timerfd、eventfd 三路 fd 进同一个 epoll。
#define _GNU_SOURCE

#include <pthread.h>
#include <signal.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sys/epoll.h>
#include <sys/eventfd.h>
#include <sys/signalfd.h>
#include <sys/timerfd.h>
#include <unistd.h>

static int efd;

static void *waker(void *arg) {
    (void)arg;
    struct timespec ts = {0, 500 * 1000 * 1000};
    nanosleep(&ts, NULL);
    uint64_t one = 1;
    ssize_t n = write(efd, &one, sizeof one);
    (void)n;
    printf("worker: write eventfd\n");
    return NULL;
}

int main(void) {
    sigset_t mask;
    sigemptyset(&mask);
    sigaddset(&mask, SIGTERM);
    if (pthread_sigmask(SIG_BLOCK, &mask, NULL) != 0) return 1;

    int ep = epoll_create1(EPOLL_CLOEXEC);
    efd = eventfd(0, EFD_NONBLOCK | EFD_CLOEXEC);
    int tfd = timerfd_create(CLOCK_MONOTONIC, TFD_NONBLOCK | TFD_CLOEXEC);
    int sfd = signalfd(-1, &mask, SFD_NONBLOCK | SFD_CLOEXEC);
    if (ep < 0 || efd < 0 || tfd < 0 || sfd < 0) return 1;

    struct epoll_event ev;
    memset(&ev, 0, sizeof ev);
    ev.events = EPOLLIN;
    ev.data.fd = tfd;
    epoll_ctl(ep, EPOLL_CTL_ADD, tfd, &ev);
    ev.data.fd = efd;
    epoll_ctl(ep, EPOLL_CTL_ADD, efd, &ev);
    ev.data.fd = sfd;
    epoll_ctl(ep, EPOLL_CTL_ADD, sfd, &ev);

    struct itimerspec its;
    memset(&its, 0, sizeof its);
    its.it_value.tv_nsec = 100 * 1000 * 1000;
    timerfd_settime(tfd, 0, &its, NULL);

    pthread_t th;
    pthread_create(&th, NULL, waker, NULL);

    for (;;) {
        struct epoll_event out[4];
        int n = epoll_wait(ep, out, 4, -1);
        if (n < 0) continue;
        for (int i = 0; i < n; ++i) {
            if (out[i].data.fd == tfd) {
                uint64_t count = 0;
                ssize_t r = read(tfd, &count, sizeof count);
                printf("timerfd: %llu 次到期\n", (unsigned long long)count);
                (void)r;
            } else if (out[i].data.fd == efd) {
                uint64_t count = 0;
                ssize_t r = read(efd, &count, sizeof count);
                printf("eventfd: 计数 %llu，循环被唤醒\n", (unsigned long long)count);
                (void)r;
                kill(getpid(), SIGTERM);
            } else if (out[i].data.fd == sfd) {
                struct signalfd_siginfo info;
                ssize_t r = read(sfd, &info, sizeof info);
                if (r == (ssize_t)sizeof info) {
                    printf("signalfd: 收到信号 %u\n", info.ssi_signo);
                    if (info.ssi_signo == SIGTERM) goto done;
                }
            }
        }
    }
done:
    pthread_join(th, NULL);
    close(sfd);
    close(tfd);
    close(efd);
    close(ep);
    printf("exit 0\n");
    return 0;
}
