// 子进程持锁后 _exit，父进程的 lock 得到 EOWNERDEAD；consistent 之后继续用。
#define _POSIX_C_SOURCE 200809L
#include <errno.h>
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
    if (pthread_mutexattr_init(&attr) != 0) return 1;
    // 两个属性缺一不可：跨进程共享，且持锁者死亡后能查出 EOWNERDEAD。
    if (pthread_mutexattr_setpshared(&attr, PTHREAD_PROCESS_SHARED) != 0) return 1;
    if (pthread_mutexattr_setrobust(&attr, PTHREAD_MUTEX_ROBUST) != 0) return 1;
    if (pthread_mutex_init(&shared->lock, &attr) != 0) return 1;
    pthread_mutexattr_destroy(&attr);

    pid_t pid = fork();
    if (pid < 0) {
        perror("fork");
        return 1;
    }
    if (pid == 0) {
        int rc = pthread_mutex_lock(&shared->lock);
        if (rc != 0) {
            fprintf(stderr, "child lock: %s\n", strerror(rc));
            _exit(1);
        }
        shared->value = 1;
        printf("child: 拿到锁，value=1，不解锁就 _exit\n");
        fflush(stdout);
        _exit(0);  // 持锁死亡：锁留在“owner dead”状态
    }

    int status = 0;
    waitpid(pid, &status, 0);

    int rc = pthread_mutex_lock(&shared->lock);
    printf("parent: lock 返回 %d (%s)\n", rc, rc == EOWNERDEAD ? "EOWNERDEAD" : "其他");
    if (rc == EOWNERDEAD) {
        // 只有调用 consistent 之后，这把锁才能再正常使用。
        if (pthread_mutex_consistent(&shared->lock) != 0) {
            perror("pthread_mutex_consistent");
            return 1;
        }
        printf("parent: 修好临界区，value=%d\n", shared->value);
        shared->value = 2;
    }
    if (pthread_mutex_unlock(&shared->lock) != 0) return 1;

    rc = pthread_mutex_lock(&shared->lock);
    printf("parent: 再次 lock 返回 %d，value=%d\n", rc, shared->value);
    pthread_mutex_unlock(&shared->lock);
    pthread_mutex_destroy(&shared->lock);
    munmap(shared, sizeof(struct Shared));
    return 0;
}
