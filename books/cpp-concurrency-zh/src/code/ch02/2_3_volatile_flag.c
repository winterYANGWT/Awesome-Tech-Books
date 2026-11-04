// mode: race
// expect: data race
// timeout: 10
#define _POSIX_C_SOURCE 200809L

#include <pthread.h>
#include <stdio.h>
#include <time.h>

static volatile int ready = 0;
static int data = 0;

static void* writer(void* arg) {
    (void)arg;
    struct timespec ts = {0, 10 * 1000 * 1000};  // 让读线程先自旋起来
    nanosleep(&ts, NULL);
    data = 42;
    ready = 1;
    return NULL;
}

static void* reader(void* arg) {
    (void)arg;
    while (ready == 0) {
    }
    printf("reader saw data=%d\n", data);
    return NULL;
}

int main(void) {
    pthread_t t0;
    pthread_t t1;
    pthread_create(&t1, NULL, reader, NULL);
    pthread_create(&t0, NULL, writer, NULL);
    pthread_join(t0, NULL);
    pthread_join(t1, NULL);
    return 0;
}
