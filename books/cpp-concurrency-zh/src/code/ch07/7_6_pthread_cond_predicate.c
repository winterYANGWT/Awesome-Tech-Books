// mode: run
// expect: 消费者：wait 返回 0，ready=1\n消费者退出
// 同一场景改成 while (!ready) pthread_cond_wait(...)：
// 先置 ready 再 signal，即使 signal 早到，谓词也能兜住。
#define _POSIX_C_SOURCE 200809L
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t cond = PTHREAD_COND_INITIALIZER;
static int ready;

static void die(const char *what, int rc) {
    fprintf(stderr, "%s: %s\n", what, strerror(rc));
    exit(1);
}

static void *consumer(void *arg) {
    (void)arg;
    int rc = pthread_mutex_lock(&mutex);
    if (rc != 0) die("pthread_mutex_lock", rc);
    while (!ready) {
        rc = pthread_cond_wait(&cond, &mutex);
        if (rc != 0) die("pthread_cond_wait", rc);
    }
    printf("消费者：wait 返回 0，ready=%d\n", ready);
    rc = pthread_mutex_unlock(&mutex);
    if (rc != 0) die("pthread_mutex_unlock", rc);
    printf("消费者退出\n");
    return NULL;
}

int main(void) {
    pthread_t tid;
    int rc = pthread_create(&tid, NULL, consumer, NULL);
    if (rc != 0) die("pthread_create", rc);

    struct timespec delay = {0, 50 * 1000 * 1000};
    nanosleep(&delay, NULL);

    rc = pthread_mutex_lock(&mutex);
    if (rc != 0) die("pthread_mutex_lock", rc);
    ready = 1;  // 先改共享状态
    rc = pthread_cond_signal(&cond);  // 再通知
    if (rc != 0) die("pthread_cond_signal", rc);
    rc = pthread_mutex_unlock(&mutex);
    if (rc != 0) die("pthread_mutex_unlock", rc);

    rc = pthread_join(tid, NULL);
    if (rc != 0) die("pthread_join", rc);
    return 0;
}
