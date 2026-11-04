// mode: run
// 子进程退出后父进程不 wait，/proc/<pid>/stat 的状态字段是 Z；wait 之后才消失。
#define _POSIX_C_SOURCE 200809L

#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

static void nap_ms(long ms)
{
    struct timespec ts;

    ts.tv_sec = ms / 1000;
    ts.tv_nsec = (ms % 1000) * 1000000L;
    while (nanosleep(&ts, &ts) == -1 && errno == EINTR) {
    }
}

static char proc_state(pid_t pid)
{
    char path[64], buf[512];
    char *end, *p;
    FILE *f;

    snprintf(path, sizeof path, "/proc/%ld/stat", (long)pid);
    f = fopen(path, "r");
    if (f == NULL) {
        return '?';
    }
    if (fgets(buf, sizeof buf, f) == NULL) {
        fclose(f);
        return '?';
    }
    fclose(f);

    end = strrchr(buf, ')');           /* comm 字段里可能有空格和括号 */
    if (end == NULL || end[1] != ' ') {
        return '?';
    }
    p = end + 2;
    return *p;
}

int main(void)
{
    pid_t pid = fork();

    if (pid == 0) {
        _exit(0);
    }
    if (pid == -1) {
        perror("fork");
        return 1;
    }

    nap_ms(200);                       /* 给子进程时间退出，父进程仍不回收 */
    printf("before wait: state=%c (zombie: %s)\n",
           proc_state(pid), proc_state(pid) == 'Z' ? "yes" : "no");

    if (waitpid(pid, NULL, 0) != pid) {
        perror("waitpid");
        return 1;
    }
    printf("waitpid reaped child: yes\n");
    printf("after wait: state=%c (stat gone: %s)\n",
           proc_state(pid), proc_state(pid) == '?' ? "yes" : "no");
    return 0;
}
