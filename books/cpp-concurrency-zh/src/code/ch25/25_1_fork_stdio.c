// fork 后子进程用 exit() 会刷新继承来的 stdio 缓冲；用 _exit() 不会。
#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <unistd.h>

int main(void) {
    // 强制全缓冲：否则输出到终端时行缓冲会在 fork 前就把这一行写出去。
    if (setvbuf(stdout, NULL, _IOFBF, 4096) != 0) {
        return 1;
    }

    printf("A: 这一行在 fork 前写进 stdio 缓冲\n");
    pid_t pid = fork();
    if (pid < 0) {
        perror("fork");
        return 1;
    }
    if (pid == 0) {
        // 子进程继承了父进程的缓冲内容，exit() 会把它再刷一遍。
        exit(0);
    }
    int status = 0;
    if (waitpid(pid, &status, 0) < 0) {
        perror("waitpid");
        return 1;
    }
    fflush(stdout);

    printf("--- 第二遍：子进程改用 _exit ---\n");
    printf("B: 这一行在 fork 前写进 stdio 缓冲\n");
    pid = fork();
    if (pid < 0) {
        perror("fork");
        return 1;
    }
    if (pid == 0) {
        // _exit() 不刷新 stdio 缓冲，也不执行 atexit 回调。
        _exit(0);
    }
    if (waitpid(pid, &status, 0) < 0) {
        perror("waitpid");
        return 1;
    }
    fflush(stdout);
    return 0;
}
