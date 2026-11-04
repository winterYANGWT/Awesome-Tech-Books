// fork 之后父子各有一份地址空间：子进程改的是自己的副本。
#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <unistd.h>

struct Item {
    int value;
};

static int g_value = 1;

int main(void) {
    struct Item *heap = malloc(sizeof *heap);
    if (heap == NULL) {
        perror("malloc");
        return 1;
    }

    g_value = 10;
    heap->value = 20;
    printf("parent before fork: g_value=%d heap->value=%d\n", g_value,
           heap->value);
    fflush(stdout);  // 免得子进程把这一行的缓冲副本也刷出来

    pid_t pid = fork();
    if (pid < 0) {
        perror("fork");
        free(heap);
        return 1;
    }

    if (pid == 0) {
        // 子进程改的是 fork 时复制出来的页，父进程看不到。
        g_value = 11;
        heap->value = 21;
        printf("child:  g_value=%d heap->value=%d\n", g_value, heap->value);
        fflush(stdout);
        _exit(0);
    }

    int status = 0;
    if (waitpid(pid, &status, 0) < 0) {
        perror("waitpid");
        free(heap);
        return 1;
    }
    printf("parent after wait: g_value=%d heap->value=%d\n", g_value,
           heap->value);

    free(heap);
    return 0;
}
