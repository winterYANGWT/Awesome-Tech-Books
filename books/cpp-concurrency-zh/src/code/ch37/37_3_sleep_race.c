// mode: race
// expect: producer: 写入 42，睡 100ms
// expect: data race
// expect: consumer
#define _POSIX_C_SOURCE 200809L

#include <pthread.h>
#include <stdio.h>
#include <time.h>

static int data;  // 普通 int：读和写之间没有 happens-before

static void sleep_ms(long ms) {
    struct timespec ts = {.tv_sec = ms / 1000, .tv_nsec = (ms % 1000) * 1000000L};
    nanosleep(&ts, NULL);
}

static void *producer(void *p) {
    (void)p;
    data = 42;
    printf("producer: 写入 42，睡 100ms\n");
    fflush(stdout);
    sleep_ms(100);
    return NULL;
}

static void *consumer(void *p) {
    (void)p;
    sleep_ms(1000);
    // 睡够 1 秒不等于生产者已经写完；这里读到 42 只是碰巧。
    printf("consumer: 1s 后读到 data=%d（看起来正确）\n", data);
    return NULL;
}

int main(void) {
    pthread_t prod;
    pthread_t cons;
    pthread_create(&prod, NULL, producer, NULL);
    pthread_create(&cons, NULL, consumer, NULL);
    pthread_join(prod, NULL);
    pthread_join(cons, NULL);
    return 0;
}
