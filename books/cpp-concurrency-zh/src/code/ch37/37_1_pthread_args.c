// mode: run
// expect: thread 0: a=20 b=22 sum=42 ret=142
// expect: thread 1: a=100 b=200 sum=300 ret=400
// expect: main: 两个线程的结果合计 542
#define _POSIX_C_SOURCE 200809L

#include <inttypes.h>
#include <pthread.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

struct arg {
    int a;
    int b;
    int sum;
    intptr_t ret;
};

#define CHECK(expr)                                                            \
    do {                                                                       \
        int rc_ = (expr);                                                      \
        if (rc_ != 0) {                                                        \
            fprintf(stderr, "%s failed: %d\n", #expr, rc_);                    \
            exit(1);                                                           \
        }                                                                      \
    } while (0)

static void *worker(void *p) {
    struct arg *arg = p;
    arg->sum = arg->a + arg->b;
    /* 返回值只能是一个 void*：先把 int 放进 intptr_t，再转指针。 */
    arg->ret = (intptr_t)(arg->sum + 100);
    return (void *)arg->ret;
}

int main(void) {
    struct arg args[2] = {
        {.a = 20, .b = 22},
        {.a = 100, .b = 200},
    };
    pthread_t tids[2];

    for (int i = 0; i < 2; ++i) {
        CHECK(pthread_create(&tids[i], NULL, worker, &args[i]));
    }

    intptr_t total = 0;
    for (int i = 0; i < 2; ++i) {
        void *ret = NULL;
        CHECK(pthread_join(tids[i], &ret));
        intptr_t value = (intptr_t)ret;
        total += value;
        printf("thread %d: a=%d b=%d sum=%d ret=%" PRIdPTR "\n",
               i, args[i].a, args[i].b, args[i].sum, value);
    }
    printf("main: 两个线程的结果合计 %" PRIdPTR "\n", total);
    return 0;
}
