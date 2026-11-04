// 父子进程共享一个放在 mmap(MAP_SHARED) 里的 sem_t：pshared=1 只是请求共享，
// 信号量本身必须位于共享内存。example.sh 编译后运行，检查退出码。
#define _POSIX_C_SOURCE 200809L
#include <errno.h>
#include <semaphore.h>
#include <stdatomic.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

struct shared_state {
    sem_t sem;          /* 必须和父子进程共享 */
    atomic_int counter; /* 被父子进程同时访问的数据 */
    atomic_int child_ready;
    atomic_int child_done;
};

static void fail(const char *what) {
    perror(what);
    exit(1);
}

int main(void) {
    struct shared_state *sh = mmap(NULL, sizeof *sh, PROT_READ | PROT_WRITE,
                                   MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    if (sh == MAP_FAILED) fail("mmap");
    if (sem_init(&sh->sem, 1, 0) == -1) fail("sem_init"); /* pshared=1：跨进程 */
    atomic_init(&sh->counter, 0);
    atomic_init(&sh->child_ready, 0);
    atomic_init(&sh->child_done, 0);

    pid_t pid = fork();
    if (pid == -1) fail("fork");
    if (pid == 0) {
        printf("子进程: 在 sem_wait 之前，counter = %d\n", atomic_load(&sh->counter));
        fflush(stdout);
        atomic_store(&sh->child_ready, 1);
        while (sem_wait(&sh->sem) == -1) {
            if (errno != EINTR) fail("sem_wait");
        }
        atomic_fetch_add(&sh->counter, 1);
        printf("子进程: 被唤醒，counter = %d\n", atomic_load(&sh->counter));
        fflush(stdout);
        atomic_store(&sh->child_done, 1);
        _exit(0); /* 子进程用 _exit，不刷新父进程的 stdio 缓冲 */
    }

    while (atomic_load(&sh->child_ready) == 0) {
        struct timespec ts = {0, 1000000L};
        nanosleep(&ts, NULL);
    }
    int value = -1;
    sem_getvalue(&sh->sem, &value);
    printf("父进程: 子进程已准备，post 之前计数 = %d\n", value);
    fflush(stdout);
    if (sem_post(&sh->sem) == -1) fail("sem_post");

    while (atomic_load(&sh->child_done) == 0) {
        struct timespec ts = {0, 1000000L};
        nanosleep(&ts, NULL);
    }
    int status = 0;
    if (waitpid(pid, &status, 0) == -1) fail("waitpid");
    printf("父进程: 子进程退出，counter = %d\n", atomic_load(&sh->counter));

    if (sem_destroy(&sh->sem) == -1) fail("sem_destroy");
    if (munmap(sh, sizeof *sh) == -1) fail("munmap");
    return 0;
}
