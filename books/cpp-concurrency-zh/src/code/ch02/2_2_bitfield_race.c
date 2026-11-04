// mode: race
// expect: data race
#define _POSIX_C_SOURCE 200809L

#include <pthread.h>
#include <stdio.h>

// 两个位域相邻，按 C++ 的对象模型共享同一个内存位置（一个 4 字节单元）。
struct Flags {
    unsigned a : 1;
    unsigned b : 1;
};

static struct Flags flags;
static pthread_barrier_t barrier;

static void* set_a(void* arg) {
    (void)arg;
    pthread_barrier_wait(&barrier);
    flags.a = 1;  // 读—改—写整个 4 字节单元
    return NULL;
}

static void* set_b(void* arg) {
    (void)arg;
    pthread_barrier_wait(&barrier);
    flags.b = 1;  // 同一个 4 字节单元
    return NULL;
}

int main(void) {
    pthread_t t0;
    pthread_t t1;
    pthread_barrier_init(&barrier, NULL, 3);
    pthread_create(&t0, NULL, set_a, NULL);
    pthread_create(&t1, NULL, set_b, NULL);
    pthread_barrier_wait(&barrier);
    pthread_join(t0, NULL);
    pthread_join(t1, NULL);
    pthread_barrier_destroy(&barrier);
    printf("a=%u b=%u sizeof(Flags)=%zu\n", flags.a, flags.b, sizeof(struct Flags));
    return 0;
}
