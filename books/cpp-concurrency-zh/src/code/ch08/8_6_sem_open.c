// mode: run
// expect: 子进程: sem_wait 成功（名字已经 unlink）
// 命名信号量：父进程 sem_open 创建，子进程按同一个名字打开；父进程 unlink 之后，
// 已经打开的信号量仍然有效，子进程照样能等到。
#define _POSIX_C_SOURCE 200809L
#include <errno.h>
#include <fcntl.h>
#include <semaphore.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

static void fail(const char *what) {
    perror(what);
    exit(1);
}

int main(void) {
    char name[64];
    snprintf(name, sizeof name, "/ch08_sem_open_%d", (int)getpid());

    sem_t *parent_sem = sem_open(name, O_CREAT | O_EXCL, 0600, 0);
    if (parent_sem == SEM_FAILED) fail("sem_open create");
    printf("父进程: 创建命名信号量（以 / 开头，不含其他 /），初值 0\n");
    fflush(stdout);

    int ready[2];
    if (pipe(ready) == -1) fail("pipe");
    pid_t pid = fork();
    if (pid == -1) fail("fork");
    if (pid == 0) {
        close(ready[0]);
        sem_t *child_sem = sem_open(name, 0);  /* 按名字打开同一个信号量 */
        if (child_sem == SEM_FAILED) fail("sem_open child");
        printf("子进程: 按名字打开同一个信号量\n");
        fflush(stdout);
        char byte = 'x';
        if (write(ready[1], &byte, 1) != 1) fail("write");
        while (sem_wait(child_sem) == -1) {
            if (errno != EINTR) fail("sem_wait");
        }
        printf("子进程: sem_wait 成功（名字已经 unlink）\n");
        fflush(stdout);
        if (sem_close(child_sem) == -1) fail("sem_close child");
        _exit(0);
    }

    close(ready[1]);
    char byte;
    if (read(ready[0], &byte, 1) != 1) fail("read");
    if (sem_unlink(name) == -1) fail("sem_unlink");
    printf("父进程: sem_unlink 返回 0，名字从系统里删除\n");
    if (sem_post(parent_sem) == -1) fail("sem_post");
    fflush(stdout);

    int status = 0;
    if (waitpid(pid, &status, 0) == -1) fail("waitpid");
    printf("父进程: 子进程退出，信号量随最后一个 close 消失\n");
    if (sem_close(parent_sem) == -1) fail("sem_close parent");
    return 0;
}
