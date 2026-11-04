// mode: hang
// expect: main: join（取消被禁用，等不到）
// expect: 仍未结束
// 工作线程关闭取消后进入死循环；pthread_cancel 只把请求排队，pthread_join 一直等。
#define _POSIX_C_SOURCE 200809L

#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>

static pthread_mutex_t mu = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t cv = PTHREAD_COND_INITIALIZER;
static int cancel_disabled;

#define CHECK(expr)                                                            \
    do {                                                                       \
        int rc_ = (expr);                                                      \
        if (rc_ != 0) {                                                        \
            fprintf(stderr, "%s failed: %d\n", #expr, rc_);                    \
            exit(1);                                                           \
        }                                                                      \
    } while (0)

static void *worker(void *p) {
    (void)p;
    CHECK(pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, NULL));

    pthread_mutex_lock(&mu);
    cancel_disabled = 1;
    printf("worker: 取消已禁用，进入死循环\n");
    fflush(stdout);
    pthread_cond_signal(&cv);
    pthread_mutex_unlock(&mu);

    volatile unsigned long counter = 0;
    for (;;) {
        counter = counter + 1;  // 没有 cancellation point，取消请求不会生效
    }
    return NULL;
}

int main(void) {
    pthread_t t;
    CHECK(pthread_create(&t, NULL, worker, NULL));

    pthread_mutex_lock(&mu);
    while (!cancel_disabled) {
        pthread_cond_wait(&cv, &mu);
    }
    pthread_mutex_unlock(&mu);

    printf("main: 请求取消\n");
    fflush(stdout);
    CHECK(pthread_cancel(t));
    printf("main: join（取消被禁用，等不到）\n");
    fflush(stdout);
    CHECK(pthread_join(t, NULL));

    printf("main: 不会到这里\n");
    return 0;
}
