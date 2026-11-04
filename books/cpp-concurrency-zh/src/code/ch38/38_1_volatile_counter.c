// mode: race
// expect: data race
// timeout: 10
#define _POSIX_C_SOURCE 200809L
#include <pthread.h>
#include <stdio.h>

// volatile 只保证访问不被优化掉；counter++ 仍然是读-改-写三步。
static volatile long counter = 0;
static const long kIters = 1000000;

static void *worker(void *arg) {
    (void)arg;
    for (long i = 0; i < kIters; ++i) {
        ++counter;
    }
    return NULL;
}

int main(void) {
    puts("两个线程各对 volatile long counter 自增 1000000 次");
    pthread_t a, b;
    if (pthread_create(&a, NULL, worker, NULL) != 0) return 1;
    if (pthread_create(&b, NULL, worker, NULL) != 0) return 1;
    if (pthread_join(a, NULL) != 0) return 1;
    if (pthread_join(b, NULL) != 0) return 1;
    printf("counter = %ld\n", counter);
    return 0;
}
