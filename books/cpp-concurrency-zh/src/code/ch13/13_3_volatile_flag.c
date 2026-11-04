// mode: race
// expect: data race
#define _POSIX_C_SOURCE 200809L
#include <pthread.h>
#include <stdio.h>
#include <time.h>

/* volatile 只阻止编译器把这个变量完全优化掉，不提供原子性，
   也不建立 release/acquire 的先行发生关系。 */
volatile int ready = 0;
int data = 0;

static void *writer(void *arg) {
    (void)arg;
    data = 42;
    ready = 1;
    return NULL;
}

static void *reader(void *arg) {
    (void)arg;
    while (!ready) {
        /* 自旋等标志 */
    }
    printf("data = %d\n", data);
    return NULL;
}

int main(void) {
    pthread_t w, r;
    pthread_create(&r, NULL, reader, NULL);
    struct timespec pause = {0, 1000000};
    nanosleep(&pause, NULL); /* 让读线程先进入自旋，稳定复现 */
    pthread_create(&w, NULL, writer, NULL);
    pthread_join(w, NULL);
    pthread_join(r, NULL);
    return 0;
}
