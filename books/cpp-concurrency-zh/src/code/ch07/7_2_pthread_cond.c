// mode: run
// expect: 唤醒 1：返回 0，ready=0\n唤醒 2：返回 0，ready=1\n消费者退出
// 用 pthread_cond_signal 制造一次谓词仍为假的唤醒。
// 两次 wait 都返回 0，返回值区分不了“真唤醒”和“假唤醒”，只能重新检查 ready。
#define _POSIX_C_SOURCE 200809L
#include <pthread.h>
#include <sched.h>
#include <stdatomic.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t cond = PTHREAD_COND_INITIALIZER;
static int ready;
static atomic_int stage;
static atomic_int woke;

static void die(const char *what, int rc) {
    fprintf(stderr, "%s: %s\n", what, strerror(rc));
    exit(1);
}

// ANCHOR: consumer
static void *consumer(void *arg) {
    (void)arg;
    int rc = pthread_mutex_lock(&mutex);
    if (rc != 0) die("pthread_mutex_lock", rc);

    atomic_store(&stage, 1);
    while (!ready) {
        rc = pthread_cond_wait(&cond, &mutex);
        if (rc != 0) die("pthread_cond_wait", rc);
        if (atomic_load(&woke) == 0) {
            printf("唤醒 1：返回 %d，ready=%d\n", rc, ready);
            atomic_store(&woke, 1);
        } else {
            printf("唤醒 2：返回 %d，ready=%d\n", rc, ready);
        }
    }
    rc = pthread_mutex_unlock(&mutex);
    if (rc != 0) die("pthread_mutex_unlock", rc);
    printf("消费者退出\n");
    return NULL;
}
// ANCHOR_END: consumer

int main(void) {
    pthread_t tid;
    int rc = pthread_create(&tid, NULL, consumer, NULL);
    if (rc != 0) die("pthread_create", rc);

    while (atomic_load(&stage) != 1) {
        sched_yield();
    }

    // 拿到锁说明消费者已经在 cond 上等待；signal 一次，但 ready 仍为 0。
    rc = pthread_mutex_lock(&mutex);
    if (rc != 0) die("pthread_mutex_lock", rc);
    rc = pthread_cond_signal(&cond);
    if (rc != 0) die("pthread_cond_signal", rc);
    rc = pthread_mutex_unlock(&mutex);
    if (rc != 0) die("pthread_mutex_unlock", rc);

    while (atomic_load(&woke) == 0) {
        sched_yield();
    }

    rc = pthread_mutex_lock(&mutex);
    if (rc != 0) die("pthread_mutex_lock", rc);
    ready = 1;
    rc = pthread_cond_signal(&cond);
    if (rc != 0) die("pthread_cond_signal", rc);
    rc = pthread_mutex_unlock(&mutex);
    if (rc != 0) die("pthread_mutex_unlock", rc);

    rc = pthread_join(tid, NULL);
    if (rc != 0) die("pthread_join", rc);
    return 0;
}
