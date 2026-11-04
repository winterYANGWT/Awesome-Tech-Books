// expect: first read=3 second read=0
// 父进程写 3 个字节后关掉写端，子进程先读到 3 字节，再读到 0（EOF）。
#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

int main(void) {
    int fd[2];
    if (pipe(fd) < 0) {
        perror("pipe");
        return 1;
    }

    pid_t pid = fork();
    if (pid < 0) {
        perror("fork");
        return 1;
    }

    if (pid == 0) {
        // 子进程只读：必须关掉自己的写端，否则写端永远有人持有，读不到 EOF。
        close(fd[1]);
        char buf[8];
        ssize_t first = read(fd[0], buf, sizeof buf);
        ssize_t second = read(fd[0], buf, sizeof buf);
        printf("child: first read=%zd second read=%zd\n", first, second);
        fflush(stdout);
        close(fd[0]);
        _exit(0);
    }

    // 父进程只写：关掉读端。
    close(fd[0]);
    const char *msg = "abc";
    if (write(fd[1], msg, strlen(msg)) != (ssize_t)strlen(msg)) {
        perror("write");
        return 1;
    }
    close(fd[1]);  // 最后一个写端关闭，子进程的下一次 read 才返回 0

    int status = 0;
    waitpid(pid, &status, 0);
    return 0;
}
