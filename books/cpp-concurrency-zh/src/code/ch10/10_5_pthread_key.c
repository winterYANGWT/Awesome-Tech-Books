// mode: run
// output: varies
// expect: destructor 调用次数: 3[\s\S]*PTHREAD_KEYS_MAX: 1024[\s\S]*PTHREAD_DESTRUCTOR_ITERATIONS: 4
#define _POSIX_C_SOURCE 200809L
#include <limits.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>

static pthread_key_t key;
static pthread_mutex_t print_mu = PTHREAD_MUTEX_INITIALIZER;
static int dtor_calls = 0;

typedef struct {
    int id;
} payload_t;

// ANCHOR: dtor
// destructor 只对非空值调用；这里负责 free，并打印是哪个线程。
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
    if (id != 3) {  // 线程 3 故意不设值
        payload_t *p = malloc(sizeof *p);
        if (p == NULL) return NULL;
        p->id = id;
        pthread_setspecific(key, p);
    }
    return NULL;
}

int main(void) {
    if (pthread_key_create(&key, destroy_payload) != 0) return 1;
    pthread_t threads[4];
    int ids[4] = {0, 1, 2, 3};
    for (int i = 0; i < 4; ++i) {
        if (pthread_create(&threads[i], NULL, worker, &ids[i]) != 0) return 1;
    }
    for (int i = 0; i < 4; ++i) pthread_join(threads[i], NULL);

    printf("destructor 调用次数: %d\n", dtor_calls);
    printf("未设置键的线程: 3\n");
    printf("PTHREAD_KEYS_MAX: %d\n", PTHREAD_KEYS_MAX);
    printf("PTHREAD_DESTRUCTOR_ITERATIONS: %d\n", PTHREAD_DESTRUCTOR_ITERATIONS);
    pthread_key_delete(key);
    pthread_mutex_destroy(&print_mu);
    return 0;
}
