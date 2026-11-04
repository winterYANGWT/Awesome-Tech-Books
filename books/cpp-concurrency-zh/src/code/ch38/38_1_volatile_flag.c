// mode: race
// expect: data race
// timeout: 10
#define _POSIX_C_SOURCE 200809L
#include <pthread.h>
#include <sched.h>
#include <stdio.h>

// volatile 让每次访问都真的发生，但不提供原子性，也不建立 happens-before。
static volatile int ready = 0;
static int data = 0;

static void *producer(void *arg) {
    (void)arg;
    data = 42;
    ready = 1;
    return NULL;
}

static void *consumer(void *arg) {
    (void)arg;
    for (int i = 0; i < 1000000 && !ready; ++i) {
        (void)sched_yield();
    }
    if (ready) {
        printf("consumer: ready, data = %d\n", data);
    } else {
        printf("consumer: ready 一直是 0\n");
    }
    return NULL;
}

int main(void) {
    puts("volatile 标志：生产者写 data 和 ready，消费者自旋等 ready");
    pthread_t p, c;
    if (pthread_create(&p, NULL, producer, NULL) != 0) return 1;
    if (pthread_create(&c, NULL, consumer, NULL) != 0) return 1;
    if (pthread_join(p, NULL) != 0) return 1;
    if (pthread_join(c, NULL) != 0) return 1;
    return 0;
}
