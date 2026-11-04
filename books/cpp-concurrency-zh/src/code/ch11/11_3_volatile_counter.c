// mode: race
// expect: data race
// volatile 只阻止编译器把访问优化掉；两个线程的 ++ 仍然没有先行发生关系。
#define _POSIX_C_SOURCE 200809L

#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static volatile long counter = 0;

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
    for (long i = 0; i < 1000000; ++i) {
        ++counter;
    }
    return NULL;
}

int main(void) {
    pthread_t a;
    pthread_t b;
    printf("期望 counter = 2000000\n");
    CHECK(pthread_create(&a, NULL, worker, NULL));
    CHECK(pthread_create(&b, NULL, worker, NULL));
    CHECK(pthread_join(a, NULL));
    CHECK(pthread_join(b, NULL));
    printf("counter = %ld\n", counter);
    return 0;
}
