// mode: run
// output: varies
// 100ms 周期 timerfd，读三次；每次 read 的 uint64_t 是上次读之后的过期次数。
#define _GNU_SOURCE

#include <errno.h>
#include <poll.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/timerfd.h>
#include <unistd.h>

int main(void)
{
    int tfd = timerfd_create(CLOCK_MONOTONIC, TFD_CLOEXEC | TFD_NONBLOCK);
    struct itimerspec its;
    uint64_t total = 0;

    if (tfd == -1) {
        perror("timerfd_create");
        return 1;
    }
    memset(&its, 0, sizeof its);
    its.it_interval.tv_nsec = 100 * 1000 * 1000;   /* 100ms */
    its.it_value = its.it_interval;
    if (timerfd_settime(tfd, 0, &its, NULL) == -1) {
        perror("timerfd_settime");
        return 1;
    }

    for (int i = 1; i <= 3; i++) {
        struct pollfd pfd = {.fd = tfd, .events = POLLIN};
        uint64_t expirations = 0;
        ssize_t n;

        if (poll(&pfd, 1, -1) == -1) {
            perror("poll");
            return 1;
        }
        n = read(tfd, &expirations, sizeof expirations);
        if (n != (ssize_t)sizeof expirations) {
            perror("read");
            return 1;
        }
        total += expirations;
        printf("read #%d: expirations=%llu\n",
               i, (unsigned long long)expirations);
    }

    printf("reads=3, total expirations=%llu\n", (unsigned long long)total);
    close(tfd);
    return 0;
}
