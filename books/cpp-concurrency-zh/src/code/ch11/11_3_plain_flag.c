// mode: race
// expect: data race
// 写线程置 ready，读线程空转等它：两个普通 bool 访问之间没有同步。
#define _POSIX_C_SOURCE 200809L

#include <pthread.h>
#include <sched.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static bool ready = false;

#define CHECK(call)                                                      \
    do {                                                                 \
        const int rc_ = (call);                                          \
        if (rc_ != 0) {                                                  \
            fprintf(stderr, "%s 失败：%s\n", #call, strerror(rc_));      \
            exit(EXIT_FAILURE);                                          \
        }                                                                \
    } while (0)

static void *writer(void *arg) {
    (void)arg;
    ready = true;
    return NULL;
}

static void *reader(void *arg) {
    (void)arg;
    while (!ready) {
        sched_yield();  // 只是让出 CPU；它不建立任何可见性保证
    }
    puts("reader 看到了 ready");
    return NULL;
}

int main(void) {
    pthread_t w;
    pthread_t r;
    printf("读线程等待 ready 变成 true\n");
    CHECK(pthread_create(&w, NULL, writer, NULL));
    CHECK(pthread_create(&r, NULL, reader, NULL));
    CHECK(pthread_join(w, NULL));
    CHECK(pthread_join(r, NULL));
    return 0;
}
