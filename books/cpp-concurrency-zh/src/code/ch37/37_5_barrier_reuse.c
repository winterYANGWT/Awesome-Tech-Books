// mode: hang
// expect: 到达第 2 轮
// expect: 仍未结束
// 手写屏障只数到 N 就广播，既不复位 count，也没有 generation；第二轮 count 已经大于 N。
#define _POSIX_C_SOURCE 200809L

#include <pthread.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#define N 3

static pthread_mutex_t mu = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t cv = PTHREAD_COND_INITIALIZER;
static int count;

#define CHECK(expr)                                                            \
    do {                                                                       \
        int rc_ = (expr);                                                      \
        if (rc_ != 0) {                                                        \
            fprintf(stderr, "%s failed: %d\n", #expr, rc_);                    \
            exit(1);                                                           \
        }                                                                      \
    } while (0)

static void barrier_wait(void) {
    pthread_mutex_lock(&mu);
    ++count;
    if (count == N) {
        pthread_cond_broadcast(&cv);
    } else {
        while (count != N) {
            pthread_cond_wait(&cv, &mu);
        }
    }
    pthread_mutex_unlock(&mu);
}

static void *worker(void *p) {
    int id = (int)(intptr_t)p;
    for (int round = 1; round <= 2; ++round) {
        printf("thread %d: 到达第 %d 轮\n", id, round);
        fflush(stdout);
        barrier_wait();
        printf("thread %d: 通过第 %d 轮\n", id, round);
        fflush(stdout);
    }
    return NULL;
}

int main(void) {
    pthread_t t[N];
    for (int i = 0; i < N; ++i) {
        CHECK(pthread_create(&t[i], NULL, worker, (void *)(intptr_t)i));
    }
    CHECK(pthread_join(t[0], NULL));
    printf("main: 不会到这里\n");
    return 0;
}
