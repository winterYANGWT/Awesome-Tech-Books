// mode: run
// output: varies
// expect: 两轮都通过，剩余许可：第一轮 0，第二轮 0
// 两份信号量做两轮屏障：count_sem 保护计数，每一轮用自己的 barrier_sem。
#define _POSIX_C_SOURCE 200809L
#include <errno.h>
#include <pthread.h>
#include <semaphore.h>
#include <stdio.h>
#include <stdlib.h>

enum { N = 4, ROUNDS = 2 };

static sem_t count_sem;        /* 初值 1，保护 counter */
static sem_t round_sem[ROUNDS];/* 每轮一个，初值 0 */
static int counter;
static pthread_mutex_t print_mutex = PTHREAD_MUTEX_INITIALIZER;

static void check(int rc, const char *what) {
    if (rc == -1) {
        perror(what);
        exit(1);
    }
}

static void wait_sem(sem_t *sem) {
    while (sem_wait(sem) == -1) {
        if (errno != EINTR) {
            perror("sem_wait");
            exit(1);
        }
    }
}

static void barrier_wait(int round) {
    wait_sem(&count_sem);
    if (++counter == N) {
        counter = 0;
        check(sem_post(&count_sem), "sem_post count_sem");
        for (int i = 0; i < N - 1; ++i) {
            check(sem_post(&round_sem[round]), "sem_post round_sem");
        }
    } else {
        check(sem_post(&count_sem), "sem_post count_sem");
        wait_sem(&round_sem[round]);
    }
}

static void *worker(void *arg) {
    long id = (long)arg;
    for (int r = 0; r < ROUNDS; ++r) {
        pthread_mutex_lock(&print_mutex);
        printf("第 %d 轮：线程 %ld 到达\n", r + 1, id);
        pthread_mutex_unlock(&print_mutex);
        barrier_wait(r);
    }
    return NULL;
}

int main(void) {
    check(sem_init(&count_sem, 0, 1), "sem_init count_sem");
    for (int r = 0; r < ROUNDS; ++r) check(sem_init(&round_sem[r], 0, 0), "sem_init round_sem");

    pthread_t t[N];
    for (long i = 0; i < N; ++i) {
        int rc = pthread_create(&t[i], NULL, worker, (void *)i);
        if (rc != 0) {
            fprintf(stderr, "pthread_create: %d\n", rc);
            return 1;
        }
    }
    for (int i = 0; i < N; ++i) pthread_join(t[i], NULL);

    int left0 = -1, left1 = -1;
    sem_getvalue(&round_sem[0], &left0);
    sem_getvalue(&round_sem[1], &left1);
    printf("两轮都通过，剩余许可：第一轮 %d，第二轮 %d\n", left0, left1);

    for (int r = 0; r < ROUNDS; ++r) check(sem_destroy(&round_sem[r]), "sem_destroy round_sem");
    check(sem_destroy(&count_sem), "sem_destroy count_sem");
    return 0;
}
