// mode: run
// 两个 worker 共用一个 epoll 和一个连接：EPOLLONESHOT 保证同一时刻只有一个在处理。
#define _GNU_SOURCE
#include <errno.h>
#include <pthread.h>
#include <stdatomic.h>
#include <stdio.h>
#include <string.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <time.h>
#include <unistd.h>

static int g_ep = -1;
static int g_fd = -1;
static atomic_int g_stop = 0;
static atomic_int g_active = 0;
static atomic_int g_max = 0;
static atomic_int g_handled = 0;

static void msleep(int ms) {
    struct timespec ts = {ms / 1000, (long)(ms % 1000) * 1000000L};
    nanosleep(&ts, NULL);
}

// ANCHOR: worker
static void *worker(void *arg) {
    (void)arg;
    while (!atomic_load_explicit(&g_stop, memory_order_relaxed)) {
        struct epoll_event out[1];
        int n = epoll_wait(g_ep, out, 1, 50);
        if (n <= 0) continue;
        if (!(out[0].events & EPOLLIN)) continue;

        int now = atomic_fetch_add(&g_active, 1) + 1;
        int seen = atomic_load(&g_max);
        while (now > seen &&
               !atomic_compare_exchange_weak(&g_max, &seen, now)) {
        }

        char buf[64];
        for (;;) {
            ssize_t k = recv(g_fd, buf, sizeof buf, MSG_DONTWAIT);
            if (k > 0) {
                atomic_fetch_add(&g_handled, (int)k);
                continue;
            }
            break;
        }
        msleep(1);  // 模拟处理耗时，让两个 worker 有重叠的机会
        atomic_fetch_sub(&g_active, 1);

        // 处理完再重新武装；不重置就再也收不到这个连接的事件。
        struct epoll_event re;
        memset(&re, 0, sizeof re);
        re.events = EPOLLIN | EPOLLONESHOT;
        re.data.fd = g_fd;
        epoll_ctl(g_ep, EPOLL_CTL_MOD, g_fd, &re);
    }
    return NULL;
}

// ANCHOR_END: worker
int main(void) {
    int sv[2];
    if (socketpair(AF_UNIX, SOCK_STREAM, 0, sv) != 0) {
        perror("socketpair");
        return 1;
    }
    g_fd = sv[0];
    g_ep = epoll_create1(EPOLL_CLOEXEC);
    struct epoll_event ev;
    memset(&ev, 0, sizeof ev);
    ev.events = EPOLLIN | EPOLLONESHOT;
    ev.data.fd = g_fd;
    if (epoll_ctl(g_ep, EPOLL_CTL_ADD, g_fd, &ev) != 0) {
        perror("epoll_ctl");
        return 1;
    }

    pthread_t w1, w2;
    pthread_create(&w1, NULL, worker, NULL);
    pthread_create(&w2, NULL, worker, NULL);

    const char *chunks = "abcdefghijklmnopqrstuvwxyz0123456789";
    int sent = 0;
    for (int i = 0; i < 10; ++i) {
        ssize_t n = send(sv[1], chunks, 10, 0);
        if (n > 0) sent += (int)n;
        msleep(2);
    }
    for (int i = 0; i < 500 && atomic_load(&g_handled) < sent; ++i) msleep(2);

    atomic_store(&g_stop, 1);
    pthread_join(w1, NULL);
    pthread_join(w2, NULL);
    printf("发送 %d 字节，处理 %d 字节，最大并发处理数 %d\n",
           sent, atomic_load(&g_handled), atomic_load(&g_max));

    close(g_ep);
    close(sv[0]);
    close(sv[1]);
    return 0;
}
