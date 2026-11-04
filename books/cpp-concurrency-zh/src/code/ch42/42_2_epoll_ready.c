// mode: run
// output: varies
// tsan: no（计时示例）
// expect: ready=1 
// expect: ready=1000 
// 注册 4096 个 eventfd，分别让 1、100、1000 个就绪，测 epoll_wait 的中位耗时。
#define _GNU_SOURCE
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/epoll.h>
#include <sys/eventfd.h>
#include <time.h>
#include <unistd.h>

#define NFDS 4096
#define REPS 501

static double now_ns(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (double)ts.tv_sec * 1e9 + (double)ts.tv_nsec;
}

static int cmp_double(const void *a, const void *b) {
    double x = *(const double *)a;
    double y = *(const double *)b;
    return (x > y) - (x < y);
}

// 每次都要求返回恰好 ready 个事件；不符就返回 -1。
static double bench(int ep, int ready) {
    static struct epoll_event events[NFDS];
    static double samples[REPS];
    for (int i = 0; i < 50; i++) {
        if (epoll_wait(ep, events, NFDS, 0) != ready) {
            return -1;
        }
    }
    for (int i = 0; i < REPS; i++) {
        double t0 = now_ns();
        int n = epoll_wait(ep, events, NFDS, 0);
        double t1 = now_ns();
        if (n != ready) {
            return -1;
        }
        samples[i] = t1 - t0;
    }
    qsort(samples, REPS, sizeof(double), cmp_double);
    return samples[REPS / 2];
}

int main(void) {
    int ep = epoll_create1(EPOLL_CLOEXEC);
    if (ep < 0) {
        perror("epoll_create1");
        return 1;
    }
    int *fds = malloc(sizeof(int) * NFDS);
    if (!fds) {
        return 1;
    }
    for (int i = 0; i < NFDS; i++) {
        fds[i] = eventfd(0, EFD_NONBLOCK);
        if (fds[i] < 0) {
            perror("eventfd");
            return 1;
        }
        struct epoll_event ev = {.events = EPOLLIN, .data.fd = fds[i]};
        if (epoll_ctl(ep, EPOLL_CTL_ADD, fds[i], &ev) != 0) {
            perror("epoll_ctl");
            return 1;
        }
    }

    uint64_t one = 1;
    if (write(fds[0], &one, sizeof one) != (ssize_t)sizeof one) {
        return 1;
    }
    double t1 = bench(ep, 1);
    for (int i = 1; i < 100; i++) {
        if (write(fds[i], &one, sizeof one) != (ssize_t)sizeof one) {
            return 1;
        }
    }
    double t100 = bench(ep, 100);
    for (int i = 100; i < 1000; i++) {
        if (write(fds[i], &one, sizeof one) != (ssize_t)sizeof one) {
            return 1;
        }
    }
    double t1000 = bench(ep, 1000);

    printf("registered=%d ready=1 median_ns=%.0f\n", NFDS, t1);
    printf("registered=%d ready=100 median_ns=%.0f\n", NFDS, t100);
    printf("registered=%d ready=1000 median_ns=%.0f\n", NFDS, t1000);
    printf("ratio 100/1 = %.1f, 1000/1 = %.1f\n", t100 / t1, t1000 / t1);
    for (int i = 0; i < NFDS; i++) {
        close(fds[i]);
    }
    close(ep);
    free(fds);
    return 0;
}
