// mode: race
// 先启用 Libevent 的 pthread 支持，再让工作线程 event_add；库内部安全了，
// flags: -levent -levent_pthreads
// 但 counter 是我们自己的普通 int，回调和两个线程同时改它，TSan 会报竞争。
#define _POSIX_C_SOURCE 200809L
#include <event2/event.h>
#include <event2/thread.h>

#include <pthread.h>
#include <stdio.h>

static struct event_base *base;
static int counter;  // 故意的：普通 int，库的锁不会保护它
static pthread_barrier_t barrier;
static struct event *worker_timer;

static void on_timer(evutil_socket_t fd, short what, void *arg) {
    (void)fd;
    (void)what;
    (void)arg;
    counter += 1;  // 在循环线程里执行
}

static void on_stop(evutil_socket_t fd, short what, void *arg) {
    (void)fd;
    (void)what;
    (void)arg;
    event_base_loopexit(base, NULL);
}

static void *worker(void *arg) {
    (void)arg;
    pthread_barrier_wait(&barrier);
    for (int i = 0; i < 1000; ++i) counter += 1;  // 与主线程竞争
    struct timeval tv = {0, 1000};
    // 跨线程 event_add：没有 evthread_use_pthreads 时这里本身就不安全
    event_add(worker_timer, &tv);
    return NULL;
}

int main(void) {
    if (evthread_use_pthreads() != 0) return 1;
    base = event_base_new();
    if (base == NULL) return 1;

    worker_timer = evtimer_new(base, on_timer, NULL);
    struct event *stop = evtimer_new(base, on_stop, NULL);
    struct timeval stop_tv = {0, 50000};
    event_add(stop, &stop_tv);

    pthread_barrier_init(&barrier, NULL, 2);
    pthread_t tid;
    if (pthread_create(&tid, NULL, worker, NULL) != 0) return 1;
    pthread_barrier_wait(&barrier);
    for (int i = 0; i < 1000; ++i) counter += 1;  // 与工作线程竞争

    event_base_dispatch(base);
    pthread_join(tid, NULL);

    printf("counter=%d (期望 2000 加上定时器回调，但普通 int 会丢更新)\n",
           counter);
    event_free(stop);
    event_free(worker_timer);
    event_base_free(base);
    pthread_barrier_destroy(&barrier);
    return 0;
}
