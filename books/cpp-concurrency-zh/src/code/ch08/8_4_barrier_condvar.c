// mode: run
// expect: 代数 100，计数 0
// 互斥量 + 条件变量 + generation：4 个线程跑 100 轮，第二轮不会误放行。
#define _POSIX_C_SOURCE 200809L
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>

enum { N = 4, ROUNDS = 100 };

typedef struct {
    pthread_mutex_t mutex;
    pthread_cond_t cond;
    int count;          /* 本轮已到达的线程数 */
    unsigned generation;/* 第几轮；每轮结束时加一 */
    int n;
} barrier_t;

static void die(int rc, const char *what) {
    fprintf(stderr, "%s: %d\n", what, rc);
    exit(1);
}

static void barrier_wait(barrier_t *b) {
    int rc = pthread_mutex_lock(&b->mutex);
    if (rc != 0) die(rc, "pthread_mutex_lock");
    unsigned my_generation = b->generation;
    if (++b->count == b->n) {
        b->count = 0;
        b->generation += 1;
        rc = pthread_cond_broadcast(&b->cond);
        if (rc != 0) die(rc, "pthread_cond_broadcast");
    } else {
        while (b->generation == my_generation) {
            rc = pthread_cond_wait(&b->cond, &b->mutex);
            if (rc != 0) die(rc, "pthread_cond_wait");
        }
    }
    rc = pthread_mutex_unlock(&b->mutex);
    if (rc != 0) die(rc, "pthread_mutex_unlock");
}

static barrier_t barrier = {PTHREAD_MUTEX_INITIALIZER, PTHREAD_COND_INITIALIZER, 0, 0, N};

static void *worker(void *arg) {
    long id = (long)arg;
    for (int r = 0; r < ROUNDS; ++r) {
        barrier_wait(&barrier);
        if (id == 0 && (r == 0 || r == 49 || r == 99)) {
            pthread_mutex_lock(&barrier.mutex);
            printf("线程 0 在第 %d 轮屏障之后看到代数 %u\n", r + 1, barrier.generation);
            pthread_mutex_unlock(&barrier.mutex);
        }
    }
    return NULL;
}

int main(void) {
    pthread_t t[N];
    for (long i = 0; i < N; ++i) {
        int rc = pthread_create(&t[i], NULL, worker, (void *)i);
        if (rc != 0) die(rc, "pthread_create");
    }
    for (int i = 0; i < N; ++i) pthread_join(t[i], NULL);

    printf("每个线程通过 %d 轮\n", ROUNDS);
    printf("代数 %u，计数 %d\n", barrier.generation, barrier.count);

    pthread_cond_destroy(&barrier.cond);
    pthread_mutex_destroy(&barrier.mutex);
    return 0;
}
