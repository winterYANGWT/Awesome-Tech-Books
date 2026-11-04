// mode: run
// output: varies
// expect: __STDC_NO_THREADS__ 未定义\n析构线程 [0-2]\n[\s\S]*destructor 调用次数: 3\nTSS_DTOR_ITERATIONS: 4
#define _POSIX_C_SOURCE 200809L
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <threads.h>

static tss_t key;
static pthread_mutex_t print_mu = PTHREAD_MUTEX_INITIALIZER;
static int dtor_calls = 0;

typedef struct {
    int id;
} payload_t;

// ANCHOR: dtor
static void destroy_payload(void *p) {
    payload_t *v = p;
    pthread_mutex_lock(&print_mu);
    ++dtor_calls;
    printf("析构线程 %d\n", v->id);
    pthread_mutex_unlock(&print_mu);
    free(v);
}
// ANCHOR_END: dtor

static void *worker(void *arg) {
    const int id = *(const int *)arg;
    if (id != 3) {
        payload_t *p = malloc(sizeof *p);
        if (p == NULL) return NULL;
        p->id = id;
        tss_set(key, p);  // C11 的每线程值
    }
    return NULL;
}

int main(void) {
#ifdef __STDC_NO_THREADS__
    printf("__STDC_NO_THREADS__ 已定义\n");
    return 1;
#else
    printf("__STDC_NO_THREADS__ 未定义\n");
#endif
    if (tss_create(&key, destroy_payload) != thrd_success) return 1;

    pthread_t threads[4];
    int ids[4] = {0, 1, 2, 3};
    // 线程用 pthread 启动；tss_t 只负责存储，和用哪套接口创建线程无关。
    for (int i = 0; i < 4; ++i) {
        if (pthread_create(&threads[i], NULL, worker, &ids[i]) != 0) return 1;
    }
    for (int i = 0; i < 4; ++i) pthread_join(threads[i], NULL);

    printf("destructor 调用次数: %d\n", dtor_calls);
    printf("TSS_DTOR_ITERATIONS: %d\n", TSS_DTOR_ITERATIONS);
    tss_delete(key);
    pthread_mutex_destroy(&print_mu);
    return 0;
}
