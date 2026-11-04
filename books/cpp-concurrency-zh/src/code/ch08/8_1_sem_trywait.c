// mode: run
// expect: 第一次 sem_trywait: -1，errno=EAGAIN
// 计数为 0 时 sem_trywait 立即失败，sem_post 之后同一个调用成功。
#define _POSIX_C_SOURCE 200809L
#include <errno.h>
#include <semaphore.h>
#include <stdio.h>
#include <string.h>

int main(void) {
    sem_t sem;
    if (sem_init(&sem, 0, 0) == -1) {  /* 初值 0：没有许可 */
        perror("sem_init");
        return 1;
    }

    errno = 0;
    int rc = sem_trywait(&sem);
    printf("第一次 sem_trywait: %d，errno=%s\n", rc,
           errno == EAGAIN ? "EAGAIN" : strerror(errno));

    int value = -1;
    sem_getvalue(&sem, &value);
    printf("失败后计数仍为 %d，没有把计数减成负数\n", value);

    if (sem_post(&sem) == -1) {
        perror("sem_post");
        return 1;
    }
    sem_getvalue(&sem, &value);
    printf("sem_post 之后计数为 %d\n", value);

    rc = sem_trywait(&sem);
    sem_getvalue(&sem, &value);
    printf("第二次 sem_trywait: %d，计数回到 %d\n", rc, value);

    sem_destroy(&sem);
    return 0;
}
