// mode: hang
// expect: 仍未结束
// 为了稳定复现丢失唤醒，主线程在循环检查完队列之后、epoll_wait 之前投递任务。
#define _GNU_SOURCE

#include <pthread.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/epoll.h>

static pthread_mutex_t mu = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t checked = PTHREAD_COND_INITIALIZER;
static pthread_cond_t done = PTHREAD_COND_INITIALIZER;
static int about_to_sleep;
static int task_ready;
static int task_done;

static void *loop_main(void *arg) {
    (void)arg;
    const int ep = epoll_create1(EPOLL_CLOEXEC);
    struct epoll_event ev;
    for (;;) {
        pthread_mutex_lock(&mu);
        if (task_ready) {
            task_ready = 0;
            pthread_mutex_unlock(&mu);
            printf("loop: 执行任务\n");
            fflush(stdout);
            pthread_mutex_lock(&mu);
            task_done = 1;
            pthread_cond_broadcast(&done);
            pthread_mutex_unlock(&mu);
            continue;
        }
        about_to_sleep = 1;
        pthread_cond_broadcast(&checked);
        pthread_mutex_unlock(&mu);

        printf("loop: 队列为空，进入 epoll_wait\n");
        fflush(stdout);
        usleep(200000);  // 强制交错：让主线程在这段时间里 push
        epoll_wait(ep, &ev, 1, -1);
    }
    return NULL;
}

int main(void) {
    pthread_t loop;
    pthread_create(&loop, NULL, loop_main, NULL);

    pthread_mutex_lock(&mu);
    while (!about_to_sleep) pthread_cond_wait(&checked, &mu);
    pthread_mutex_unlock(&mu);

    printf("main: 投递任务，等它完成\n");
    fflush(stdout);
    pthread_mutex_lock(&mu);
    task_ready = 1;  // 只改队列，没有 eventfd 唤醒
    pthread_mutex_unlock(&mu);

    pthread_mutex_lock(&mu);
    while (!task_done) pthread_cond_wait(&done, &mu);
    pthread_mutex_unlock(&mu);
    printf("main: 任务完成\n");
    return 0;
}
