// mode: run
// expect: timerfd：定时到期
// expect: eventfd：收到唤醒
// expect: 自管道：收到 SIGTERM
#define _GNU_SOURCE
#define _POSIX_C_SOURCE 200809L
#include <errno.h>
#include <fcntl.h>
#include <pthread.h>
#include <signal.h>
#include <stdint.h>
#include <stdio.h>
#include <sys/epoll.h>
#include <sys/eventfd.h>
#include <sys/timerfd.h>
#include <time.h>
#include <unistd.h>

static int pipe_write = -1;

static void on_term(int signo)
{
    int saved = errno;
    unsigned char byte = (unsigned char)signo;
    if (pipe_write >= 0) {
        ssize_t n = write(pipe_write, &byte, 1);
        (void)n;
    }
    errno = saved;
}

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
    int p[2], tfd, efd, epfd;
    struct sigaction sa = {0};
    pthread_t th;
    struct helper_arg arg;

    if (pipe2(p, O_NONBLOCK | O_CLOEXEC) == -1)
        return 1;
    pipe_write = p[1];

    sa.sa_handler = on_term;
    sigemptyset(&sa.sa_mask);
    if (sigaction(SIGTERM, &sa, NULL) == -1)
        return 1;

    tfd = timerfd_create(CLOCK_MONOTONIC, TFD_NONBLOCK | TFD_CLOEXEC);
    efd = eventfd(0, EFD_NONBLOCK | EFD_CLOEXEC);
    epfd = epoll_create1(EPOLL_CLOEXEC);
    if (tfd == -1 || efd == -1 || epfd == -1)
        return 1;

    struct itimerspec its = {.it_value = {.tv_sec = 0, .tv_nsec = 30000000}};
    if (timerfd_settime(tfd, 0, &its, NULL) == -1)
        return 1;

    struct epoll_event ev = {.events = EPOLLIN};
    ev.data.fd = tfd;
    epoll_ctl(epfd, EPOLL_CTL_ADD, tfd, &ev);
    ev.data.fd = efd;
    epoll_ctl(epfd, EPOLL_CTL_ADD, efd, &ev);
    ev.data.fd = p[0];
    epoll_ctl(epfd, EPOLL_CTL_ADD, p[0], &ev);

    arg.efd = efd;
    if (pthread_create(&th, NULL, helper, &arg) != 0)
        return 1;

    struct epoll_event events[3];
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
            } else if (fd == p[0]) {
                unsigned char buf[8];
                ssize_t got = read(p[0], buf, sizeof buf);
                if (got > 0) {
                    printf("自管道：收到 %s\n",
                           buf[0] == SIGTERM ? "SIGTERM" : "其他信号");
                    ++seen;
                }
            }
        }
    }

    pthread_join(th, NULL);
    printf("三路 fd 都在同一个 epoll 里\n");
    close(epfd);
    close(tfd);
    close(efd);
    close(p[0]);
    close(p[1]);
    return 0;
}
