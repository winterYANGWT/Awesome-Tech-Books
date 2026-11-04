// mode: run
// expect: 线程 0: 环闭合，收到 5
// 5 个线程排成环：每人一个初值 0 的信号量，写下一个人的消息后 post 他的信号量。
#define _POSIX_C_SOURCE 200809L
#include <errno.h>
#include <pthread.h>
#include <semaphore.h>
#include <stdio.h>
#include <stdlib.h>

enum { N = 5 };

static sem_t sems[N];      /* sems[i]：线程 i 的信箱有信 */
static int messages[N];    /* messages[i]：发给线程 i 的数据 */

static void check(int rc, const char *what) {
    if (rc == -1) {
        perror(what);
        exit(1);
    }
}

static int wait_own(int i) {
    while (sem_wait(&sems[i]) == -1) {
        if (errno != EINTR) {
            perror("sem_wait");
            exit(1);
        }
    }
    return messages[i];
}

static void *worker(void *arg) {
    long i = (long)arg;
    if (i == 0) {
        messages[1] = 1;
        printf("线程 0: 写入 messages[1] = 1，post 信号量 1\n");
        check(sem_post(&sems[1]), "sem_post");
    }
    int v = wait_own((int)i);
    printf("线程 %ld: 等到自己的信号量，读到 %d\n", i, v);
    if (i == 0) {
        printf("线程 0: 环闭合，收到 %d\n", v);
        return NULL;
    }
    int next = (int)((i + 1) % N);
    messages[next] = v + 1;
    printf("线程 %ld: 写入 messages[%d] = %d，post 信号量 %d\n", i, next, v + 1, next);
    check(sem_post(&sems[next]), "sem_post");
    return NULL;
}

int main(void) {
    for (int i = 0; i < N; ++i) check(sem_init(&sems[i], 0, 0), "sem_init");
    pthread_t t[N];
    for (long i = 0; i < N; ++i) {
        int rc = pthread_create(&t[i], NULL, worker, (void *)i);
        if (rc != 0) {
            fprintf(stderr, "pthread_create: %d\n", rc);
            return 1;
        }
    }
    for (int i = 0; i < N; ++i) pthread_join(t[i], NULL);
    for (int i = 0; i < N; ++i) check(sem_destroy(&sems[i]), "sem_destroy");
    return 0;
}
