// mode: run
// output: varies
// expect: 每轮恰好一个串行线程: 是
// pthread_barrier_t：每轮一个线程从 pthread_barrier_wait 拿到 PTHREAD_BARRIER_SERIAL_THREAD。
#define _POSIX_C_SOURCE 200809L
#include <pthread.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

enum { N = 4, ROUNDS = 10, PER_THREAD = 1000 };

static pthread_barrier_t barrier;
static uint64_t partials[N];
static uint64_t round_total;
static int serial_count[ROUNDS];

static void die(int rc, const char *what) {
    fprintf(stderr, "%s: %d\n", what, rc);
    exit(1);
}

static void *worker(void *arg) {
    long id = (long)arg;
    for (int r = 0; r < ROUNDS; ++r) {
        uint64_t lo = (uint64_t)r * N * PER_THREAD + 1 + (uint64_t)id * PER_THREAD;
        uint64_t sum = 0;
        for (int k = 0; k < PER_THREAD; ++k) sum += lo + k;
        partials[id] = sum;

        int rc = pthread_barrier_wait(&barrier);
        if (rc == PTHREAD_BARRIER_SERIAL_THREAD) {
            uint64_t total = 0;
            for (int i = 0; i < N; ++i) total += partials[i];
            round_total = total;
            serial_count[r] += 1;
            printf("第 %2d 轮：串行线程 %ld，总和 %llu\n", r + 1, id,
                   (unsigned long long)round_total);
        } else if (rc != 0) {
            die(rc, "pthread_barrier_wait");
        }
        rc = pthread_barrier_wait(&barrier); /* 下一轮写 partials 之前再对齐一次 */
        if (rc != 0 && rc != PTHREAD_BARRIER_SERIAL_THREAD) die(rc, "pthread_barrier_wait");
    }
    return NULL;
}

int main(void) {
    printf("PTHREAD_BARRIER_SERIAL_THREAD = %d\n", PTHREAD_BARRIER_SERIAL_THREAD);
    int rc = pthread_barrier_init(&barrier, NULL, N);
    if (rc != 0) die(rc, "pthread_barrier_init");

    pthread_t t[N];
    for (long i = 0; i < N; ++i) {
        rc = pthread_create(&t[i], NULL, worker, (void *)i);
        if (rc != 0) die(rc, "pthread_create");
    }
    for (int i = 0; i < N; ++i) pthread_join(t[i], NULL);

    int ok = 1;
    for (int r = 0; r < ROUNDS; ++r) ok = ok && serial_count[r] == 1;
    printf("每轮恰好一个串行线程: %s\n", ok ? "是" : "否");

    rc = pthread_barrier_destroy(&barrier);
    if (rc != 0) die(rc, "pthread_barrier_destroy");
    printf("销毁返回 0，共 %d 轮\n", ROUNDS);
    return 0;
}
