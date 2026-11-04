// mode: run
// output: varies
// expect: ^round,sleep_ms,timerfd_ms
// 5 次 100 ms：sleep 的相对等待 vs timerfd 的绝对截止时间。
#define _GNU_SOURCE

#include <stdint.h>
#include <stdio.h>
#include <time.h>
#include <sys/timerfd.h>
#include <unistd.h>

static double ms_between(const struct timespec *a, const struct timespec *b) {
    return (double)(b->tv_sec - a->tv_sec) * 1000.0 +
           (double)(b->tv_nsec - a->tv_nsec) / 1e6;
}

int main(void) {
    double sleep_err[5];
    double timer_err[5];

    struct timespec start, now;
    clock_gettime(CLOCK_MONOTONIC, &start);
    for (int i = 0; i < 5; ++i) {
        struct timespec step = {0, 100 * 1000 * 1000};
        nanosleep(&step, NULL);
        clock_gettime(CLOCK_MONOTONIC, &now);
        sleep_err[i] = ms_between(&start, &now) - (i + 1) * 100.0;
    }

    int tfd = timerfd_create(CLOCK_MONOTONIC, TFD_CLOEXEC);
    clock_gettime(CLOCK_MONOTONIC, &start);
    for (int i = 0; i < 5; ++i) {
        struct timespec deadline = start;
        deadline.tv_nsec += 100 * 1000 * 1000;
        deadline.tv_sec += deadline.tv_nsec / 1000000000L;
        deadline.tv_nsec %= 1000000000L;
        start = deadline;  // 下一轮的绝对截止时间

        struct itimerspec its;
        its.it_value = deadline;
        its.it_interval.tv_sec = 0;
        its.it_interval.tv_nsec = 0;
        timerfd_settime(tfd, TFD_TIMER_ABSTIME, &its, NULL);
        uint64_t expirations = 0;
        ssize_t r = read(tfd, &expirations, sizeof expirations);
        (void)r;
        clock_gettime(CLOCK_MONOTONIC, &now);
        timer_err[i] = ms_between(&deadline, &now);
    }
    close(tfd);

    printf("round,sleep_ms,timerfd_ms\n");
    for (int i = 0; i < 5; ++i)
        printf("%d,%.3f,%.3f\n", i + 1, sleep_err[i], timer_err[i]);
    return 0;
}
