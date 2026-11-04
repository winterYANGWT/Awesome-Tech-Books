// mode: run
// expect: 第 2 次尝试：两把都拿到
#define _POSIX_C_SOURCE 200809L
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static pthread_mutex_t m1 = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t m2 = PTHREAD_MUTEX_INITIALIZER;

// gate 保护 stage，并用来固定“主线程先持 m2”的次序。
static pthread_mutex_t gate = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t gate_cv = PTHREAD_COND_INITIALIZER;
static int stage = 0;  // 1: 主线程持有 m2；2: 工作线程失败一次；3: m2 已释放

static void die(int rc, const char* what) {
    fprintf(stderr, "%s: %s\n", what, strerror(rc));
    exit(EXIT_FAILURE);
}

#define CHECK(call) do { int rc_ = (call); if (rc_ != 0) die(rc_, #call); } while (0)

static void set_stage(int value) {
    CHECK(pthread_mutex_lock(&gate));
    stage = value;
    CHECK(pthread_cond_broadcast(&gate_cv));
    CHECK(pthread_mutex_unlock(&gate));
}

static void wait_stage(int want) {
    CHECK(pthread_mutex_lock(&gate));
    while (stage < want) CHECK(pthread_cond_wait(&gate_cv, &gate));
    CHECK(pthread_mutex_unlock(&gate));
}

static void* worker(void* arg) {
    (void)arg;
    wait_stage(1);  // 此时主线程已经持有 m2

    for (int attempt = 1;; ++attempt) {
        int got1 = pthread_mutex_trylock(&m1) == 0;
        int got2 = got1 && pthread_mutex_trylock(&m2) == 0;

        if (got1 && got2) {
            printf("第 %d 次尝试：两把都拿到\n", attempt);
            CHECK(pthread_mutex_unlock(&m2));
            CHECK(pthread_mutex_unlock(&m1));
            return NULL;
        }
        if (got1) CHECK(pthread_mutex_unlock(&m1));

        printf("第 %d 次尝试：m2 忙，已释放 m1\n", attempt);
        if (attempt == 1) set_stage(2);  // 告诉主线程“我失败了一次”
        wait_stage(3);                   // 等主线程释放 m2 再重试
    }
}

int main(void) {
    CHECK(pthread_mutex_lock(&m2));  // 主线程先占住 m2

    pthread_t t;
    CHECK(pthread_create(&t, NULL, worker, NULL));
    set_stage(1);
    wait_stage(2);                    // 等工作线程拿到 m1、在 m2 上失败

    CHECK(pthread_mutex_unlock(&m2)); // 释放 m2，工作线程才能重试
    set_stage(3);

    CHECK(pthread_join(t, NULL));
    printf("主线程：m2 已释放，转账完成\n");
    return 0;
}
