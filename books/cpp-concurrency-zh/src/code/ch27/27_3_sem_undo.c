// 子进程 P 之后不 V 就 _exit：SEM_UNDO 会把它的 P 撤销，不带就停在 0。
#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <sys/sem.h>
#include <sys/wait.h>
#include <unistd.h>

union semun {
    int val;
    struct semid_ds *buf;
    unsigned short *array;
};

// 跑一轮：undo=1 时 P 操作带 SEM_UNDO。返回子进程退出后信号量的值。
static int trial(int undo) {
    int id = semget(IPC_PRIVATE, 1, IPC_CREAT | 0600);
    if (id < 0) {
        perror("semget");
        exit(1);
    }
    union semun arg;
    arg.val = 1;
    if (semctl(id, 0, SETVAL, arg) < 0) {
        perror("semctl SETVAL");
        exit(1);
    }

    pid_t pid = fork();
    if (pid < 0) {
        perror("fork");
        exit(1);
    }
    if (pid == 0) {
        struct sembuf op = {0, -1, undo ? SEM_UNDO : 0};
        if (semop(id, &op, 1) < 0) {
            perror("semop");
            _exit(1);
        }
        printf("child(undo=%d): P 完成，信号量应为 0，直接 _exit 不 V\n", undo);
        fflush(stdout);
        _exit(0);  // 没有 V；SEM_UNDO 由内核在进程退出时补偿
    }

    int status = 0;
    waitpid(pid, &status, 0);
    int value = semctl(id, 0, GETVAL);
    semctl(id, 0, IPC_RMID);
    return value;
}

int main(void) {
    printf("with SEM_UNDO: value=%d\n", trial(1));
    printf("without SEM_UNDO: value=%d\n", trial(0));
    return 0;
}
