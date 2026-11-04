// mode: run
// System V 共享内存：shmget 用 key 标识，父子都能 shmat，最后 shmctl(IPC_RMID) 清理。
#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <string.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/wait.h>
#include <unistd.h>

int main(void)
{
    const char text[] = "hello from child via SysV shm";
    int shmid = shmget(IPC_PRIVATE, 4096, IPC_CREAT | 0600);
    char *p;
    pid_t pid;

    if (shmid == -1) {
        perror("shmget");
        return 1;
    }
    p = shmat(shmid, NULL, 0);
    if (p == (char *)-1) {
        perror("shmat");
        return 1;
    }

    pid = fork();
    if (pid == 0) {
        memcpy(p, text, sizeof text);
        if (shmdt(p) == -1) {
            _exit(1);
        }
        _exit(0);
    }
    if (pid == -1) {
        perror("fork");
        return 1;
    }

    if (waitpid(pid, NULL, 0) != pid) {
        perror("waitpid");
        return 1;
    }
    printf("parent read: %s\n", p);
    if (shmdt(p) == -1) {
        perror("shmdt");
        return 1;
    }
    if (shmctl(shmid, IPC_RMID, NULL) == -1) {
        perror("shmctl");
        return 1;
    }
    printf("shmctl(IPC_RMID): ok\n");
    return 0;
}
