// 父子进程用 IPC_PRIVATE 信号量保护一块共享内存里的计数器，各加 100000 次。
#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <sys/sem.h>
#include <sys/wait.h>
#include <unistd.h>

#define ROUNDS 100000

union semun {
    int val;
    struct semid_ds *buf;
    unsigned short *array;
};

static void P(int id) {
    struct sembuf op = {0, -1, 0};
    if (semop(id, &op, 1) < 0) {
        perror("semop P");
        exit(1);
    }
}

static void V(int id) {
    struct sembuf op = {0, +1, 0};
    if (semop(id, &op, 1) < 0) {
        perror("semop V");
        exit(1);
    }
}

static void bump(int id, int *counter) {
    for (int i = 0; i < ROUNDS; ++i) {
        P(id);
        ++*counter;  // 临界区：读-改-写不是原子的，必须包在 P/V 之间
        V(id);
    }
}

int main(void) {
    int id = semget(IPC_PRIVATE, 1, IPC_CREAT | 0600);
    if (id < 0) {
        perror("semget");
        return 1;
    }
    union semun arg;
    arg.val = 1;  // 二值信号量，初值 1
    if (semctl(id, 0, SETVAL, arg) < 0) {
        perror("semctl SETVAL");
        return 1;
    }

    int *counter = mmap(NULL, sizeof(int), PROT_READ | PROT_WRITE,
                        MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    if (counter == MAP_FAILED) {
        perror("mmap");
        return 1;
    }
    *counter = 0;

    pid_t pid = fork();
    if (pid < 0) {
        perror("fork");
        return 1;
    }
    if (pid == 0) {
        bump(id, counter);
        _exit(0);
    }

    bump(id, counter);
    int status = 0;
    waitpid(pid, &status, 0);

    printf("counter=%d\n", *counter);
    semctl(id, 0, IPC_RMID);  // 信号量集不随进程退出消失，要显式删
    munmap(counter, sizeof(int));
    return 0;
}
