// mode: run
// expect: counter = 2000000
#define _POSIX_C_SOURCE 200809L

#include <errno.h>
#include <inttypes.h>
#include <pthread.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define THREADS 2
#define INCREMENTS 1000000

static pthread_mutex_t counter_lock = PTHREAD_MUTEX_INITIALIZER;
static uint64_t counter = 0;

// pthread 函数用返回值报告错误，不设置 errno。
#define CHECK(call)                                                      \
    do {                                                                 \
        const int rc_ = (call);                                          \
        if (rc_ != 0) {                                                  \
            fprintf(stderr, "%s 失败：%s\n", #call, strerror(rc_));      \
            exit(EXIT_FAILURE);                                          \
        }                                                                \
    } while (0)

static void *worker(void *arg) {
    (void)arg;
    for (int i = 0; i < INCREMENTS; ++i) {
        CHECK(pthread_mutex_lock(&counter_lock));
        ++counter;
        CHECK(pthread_mutex_unlock(&counter_lock));
    }
    return NULL;
}

int main(void) {
    pthread_t threads[THREADS];
    for (int i = 0; i < THREADS; ++i) {
        CHECK(pthread_create(&threads[i], NULL, worker, NULL));
    }
    for (int i = 0; i < THREADS; ++i) {
        CHECK(pthread_join(threads[i], NULL));
    }
    printf("counter = %" PRIu64 "\n", counter);
    printf("每个 pthread 调用都返回 0\n");
    return 0;
}
