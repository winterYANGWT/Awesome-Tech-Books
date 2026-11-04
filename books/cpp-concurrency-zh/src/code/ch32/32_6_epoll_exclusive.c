// mode: run
// output: varies
// 4 个线程各自一个 epoll 等同一个监听 fd，对比默认唤醒和 EPOLLEXCLUSIVE。
#define _GNU_SOURCE
#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <pthread.h>
#include <stdatomic.h>
#include <stdio.h>
#include <string.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <unistd.h>

static atomic_int g_accepted = 0;
static atomic_int g_wakeups = 0;
static atomic_int g_stop = 0;
static int g_lfd = -1;

static void *worker(void *arg) {
    int ep = (int)(long)arg;
    while (!atomic_load_explicit(&g_stop, memory_order_relaxed)) {
        struct epoll_event out[4];
        int n = epoll_wait(ep, out, 4, 50);
        if (n <= 0) continue;
        for (int i = 0; i < n; ++i) {
            if (out[i].data.fd != g_lfd) continue;
            atomic_fetch_add(&g_wakeups, 1);
            // 一次只 accept 一个：这样“唤醒次数”和“连接数”可以直接相比。
            int c = accept(g_lfd, NULL, NULL);
            if (c >= 0) {
                atomic_fetch_add(&g_accepted, 1);
                close(c);
            }
        }
    }
    return NULL;
}

static void run(int exclusive, const char *name, int total) {
    g_lfd = socket(AF_INET, SOCK_STREAM, 0);
    if (g_lfd < 0) { perror("socket"); return; }
    int yes = 1;
    setsockopt(g_lfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof yes);
    struct sockaddr_in addr;
    memset(&addr, 0, sizeof addr);
    addr.sin_family = AF_INET;
    addr.sin_port = 0;
    addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    if (bind(g_lfd, (struct sockaddr *)&addr, sizeof addr) != 0) { perror("bind"); return; }
    if (listen(g_lfd, 256) != 0) { perror("listen"); return; }
    int flags = fcntl(g_lfd, F_GETFL, 0);
    fcntl(g_lfd, F_SETFL, flags | O_NONBLOCK);

    int eps[4];
    for (int i = 0; i < 4; ++i) {
        eps[i] = epoll_create1(EPOLL_CLOEXEC);
        struct epoll_event ev;
        memset(&ev, 0, sizeof ev);
        ev.events = EPOLLIN | (exclusive ? EPOLLEXCLUSIVE : 0);
        ev.data.fd = g_lfd;
        if (epoll_ctl(eps[i], EPOLL_CTL_ADD, g_lfd, &ev) != 0) {
            perror("epoll_ctl");
            return;
        }
    }

    atomic_store(&g_accepted, 0);
    atomic_store(&g_wakeups, 0);
    atomic_store(&g_stop, 0);
    pthread_t workers[4];
    for (int i = 0; i < 4; ++i)
        pthread_create(&workers[i], NULL, worker, (void *)(long)eps[i]);

    socklen_t len = sizeof addr;
    getsockname(g_lfd, (struct sockaddr *)&addr, &len);
    for (int i = 0; i < total; ++i) {
        int fd = socket(AF_INET, SOCK_STREAM, 0);
        if (connect(fd, (struct sockaddr *)&addr, sizeof addr) != 0) {
            perror("connect");
            break;
        }
        close(fd);
        usleep(1000);  // 一个个到达，让每次状态变化可观察
    }
    for (int i = 0; i < 1000 && atomic_load(&g_accepted) < total; ++i)
        usleep(1000);

    atomic_store(&g_stop, 1);
    for (int i = 0; i < 4; ++i) pthread_join(workers[i], NULL);

    int acc = atomic_load(&g_accepted);
    int wake = atomic_load(&g_wakeups);
    printf("%s: connections=%d wakeups=%d ratio=%.2f\n", name, acc, wake,
           acc > 0 ? (double)wake / acc : 0.0);

    for (int i = 0; i < 4; ++i) close(eps[i]);
    close(g_lfd);
}

int main(void) {
    run(0, "default", 100);
    run(1, "EPOLLEXCLUSIVE", 100);
    return 0;
}
