// mode: hang
// expect: 仍未结束
// 普通互斥量：子进程持锁 _exit，父进程的 lock 永远等下去。
#define _POSIX_C_SOURCE 200809L
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/wait.h>
#include <unistd.h>

struct Shared {
    pthread_mutex_t lock;
    int value;
};

int main(void) {
    struct Shared *shared = mmap(NULL, sizeof(struct Shared), PROT_READ | PROT_WRITE,
                                 MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    if (shared == MAP_FAILED) {
        perror("mmap");
        return 1;
    }
    shared->value = 0;

    pthread_mutexattr_t attr;
    pthread_mutexattr_init(&attr);
    pthread_mutexattr_setpshared(&attr, PTHREAD_PROCESS_SHARED);
    // 没有 setrobust：默认是 PTHREAD_MUTEX_STALLED。
    pthread_mutex_init(&shared->lock, &attr);
    pthread_mutexattr_destroy(&attr);

    pid_t pid = fork();
    if (pid < 0) {
        perror("fork");
        return 1;
    }
    if (pid == 0) {
        pthread_mutex_lock(&shared->lock);
        shared->value = 1;
        printf("child: 拿到锁，不解锁就 _exit\n");
        fflush(stdout);
        _exit(0);
    }

    int status = 0;
    waitpid(pid, &status, 0);
    printf("parent: 子进程已死，现在 lock 同一把锁\n");
    fflush(stdout);

    // 不 robust 的锁没有 owner-dead 状态，这里会一直阻塞。
    pthread_mutex_lock(&shared->lock);
    printf("parent: 不会执行到这里\n");
    return 0;
}
