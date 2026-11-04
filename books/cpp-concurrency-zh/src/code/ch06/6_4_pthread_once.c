// mode: run
#define _POSIX_C_SOURCE 200809L
#include <pthread.h>
#include <stdio.h>
#include <time.h>

#define THREADS 16

static pthread_once_t g_once = PTHREAD_ONCE_INIT;
static pthread_mutex_t g_mutex = PTHREAD_MUTEX_INITIALIZER;
static int g_calls = 0;
static int g_ready = 0;

static void initialize(void) {
    const struct timespec pause = {0, 10 * 1000 * 1000};  /* 10 ms */
    pthread_mutex_lock(&g_mutex);
    ++g_calls;
    pthread_mutex_unlock(&g_mutex);
    printf("初始化开始（只应打印一次）\n");
    nanosleep(&pause, NULL);
    pthread_mutex_lock(&g_mutex);
    g_ready = 1;
    pthread_mutex_unlock(&g_mutex);
}

static void *worker(void *arg) {
    (void)arg;
    int rc = pthread_once(&g_once, initialize);  /* 返回 0；C 接口没有异常 */
    if (rc != 0) {
        fprintf(stderr, "pthread_once failed: %d\n", rc);
        return NULL;
    }
    pthread_mutex_lock(&g_mutex);
    int ready = g_ready;
    pthread_mutex_unlock(&g_mutex);
    return (void *)(long)ready;
}

int main(void) {
    pthread_t threads[THREADS];
    for (int i = 0; i < THREADS; ++i) {
        if (pthread_create(&threads[i], NULL, worker, NULL) != 0) {
            fprintf(stderr, "pthread_create failed\n");
            return 1;
        }
    }

    int ready = 0;
    for (int i = 0; i < THREADS; ++i) {
        void *ret = NULL;
        if (pthread_join(threads[i], &ret) != 0) {
            fprintf(stderr, "pthread_join failed\n");
            return 1;
        }
        ready += ret != NULL;
    }

    pthread_mutex_lock(&g_mutex);
    int calls = g_calls;
    pthread_mutex_unlock(&g_mutex);
    printf("初始化执行次数=%d\n", calls);
    printf("看到初始化完成的线程=%d / %d\n", ready, THREADS);
    return 0;
}
