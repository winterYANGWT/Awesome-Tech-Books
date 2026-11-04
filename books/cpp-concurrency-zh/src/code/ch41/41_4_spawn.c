// mode: run
// posix_spawn 不复制整个地址空间，也就没有“继承一把已加锁的 mutex”这回事。
#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <spawn.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

extern char **environ;

int main(void)
{
    posix_spawnattr_t attr;
    posix_spawn_file_actions_t actions;
    sigset_t defaults;
    char *argv[] = {(char *)"/bin/echo", (char *)"echo from posix_spawn", NULL};
    pid_t pid;
    int status = 0;

    if (posix_spawn_file_actions_init(&actions) != 0 ||
        posix_spawnattr_init(&attr) != 0) {
        return 1;
    }

    /* 子进程把 SIGUSR1 恢复成默认动作，而不是继承父进程的处理函数 */
    sigemptyset(&defaults);
    sigaddset(&defaults, SIGUSR1);
    if (posix_spawnattr_setsigdefault(&attr, &defaults) != 0 ||
        posix_spawnattr_setflags(&attr, POSIX_SPAWN_SETSIGDEF) != 0) {
        return 1;
    }

    if (posix_spawn(&pid, "/bin/echo", &actions, &attr, argv, environ) != 0) {
        perror("posix_spawn");
        return 1;
    }
    printf("spawned, pid>0: %s\n", pid > 0 ? "yes" : "no");

    if (waitpid(pid, &status, 0) != pid) {
        perror("waitpid");
        return 1;
    }
    printf("WIFEXITED=%d, exit status=%d\n",
           WIFEXITED(status) ? 1 : 0, WEXITSTATUS(status));

    posix_spawnattr_destroy(&attr);
    posix_spawn_file_actions_destroy(&actions);
    return 0;
}
