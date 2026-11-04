// mode: run
// tsan: no（本机 GCC 15/Clang 21 的 TSan 在 glibc 的 thrd_create 上崩溃，实测）
// expect: consumer: 读到 42
// expect: producer 返回 7
#include <stdio.h>
#include <threads.h>

struct shared {
    mtx_t mtx;
    cnd_t cv;
    int data;
    int ready;
};

static int producer(void *p) {
    struct shared *s = p;
    mtx_lock(&s->mtx);
    s->data = 42;
    s->ready = 1;
    cnd_signal(&s->cv);
    mtx_unlock(&s->mtx);
    return 7;
}

static int consumer(void *p) {
    struct shared *s = p;
    mtx_lock(&s->mtx);
    while (!s->ready) {
        cnd_wait(&s->cv, &s->mtx);
    }
    printf("consumer: 读到 %d\n", s->data);
    mtx_unlock(&s->mtx);
    return 0;
}

int main(void) {
    struct shared s = {0};
    if (mtx_init(&s.mtx, mtx_plain) != thrd_success) {
        return 1;
    }
    if (cnd_init(&s.cv) != thrd_success) {
        return 1;
    }

    thrd_t prod;
    thrd_t cons;
    if (thrd_create(&prod, producer, &s) != thrd_success) {
        return 1;
    }
    if (thrd_create(&cons, consumer, &s) != thrd_success) {
        return 1;
    }

    int prod_result = 0;
    int cons_result = 0;
    thrd_join(prod, &prod_result);
    thrd_join(cons, &cons_result);

    printf("producer 返回 %d（int，不是 void*）\n", prod_result);
    printf("consumer 返回 %d\n", cons_result);
    mtx_destroy(&s.mtx);
    cnd_destroy(&s.cv);
    return 0;
}
