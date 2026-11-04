// posix_spawn 用 file_actions 把子进程的 stdout 接到管道，父进程读回来。
#define _GNU_SOURCE

#include <errno.h>
#include <fcntl.h>
#include <spawn.h>
#include <stdio.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

extern char **environ;

int main(void) {
    int fds[2];
    if (pipe2(fds, O_CLOEXEC) < 0) {
        perror("pipe2");
        return 1;
    }

    posix_spawn_file_actions_t actions;
    if (posix_spawn_file_actions_init(&actions) != 0) {
        perror("posix_spawn_file_actions_init");
        return 1;
    }
    // dup2 到 fd 1 时，新 fd 的 CLOEXEC 会被清掉，子进程写的就是管道。
    int rc = posix_spawn_file_actions_adddup2(&actions, fds[1], STDOUT_FILENO);
    if (rc != 0) {
        fprintf(stderr, "adddup2: %s\n", strerror(rc));
        return 1;
    }

    char *argv[] = {(char *)"echo", (char *)"hello from posix_spawn", NULL};
    pid_t pid = -1;
    rc = posix_spawn(&pid, "/bin/echo", &actions, NULL, argv, environ);
    posix_spawn_file_actions_destroy(&actions);
    close(fds[1]);
    if (rc != 0) {
        fprintf(stderr, "posix_spawn: %s\n", strerror(rc));
        close(fds[0]);
        return 1;
    }

    char buf[256];
    size_t used = 0;
    for (;;) {
        ssize_t n = read(fds[0], buf + used, sizeof buf - 1 - used);
        if (n < 0) {
            if (errno == EINTR) {
                continue;
            }
            perror("read");
            break;
        }
        if (n == 0) {
            break;
        }
        used += (size_t)n;
        if (used == sizeof buf - 1) {
            break;
        }
    }
    buf[used] = '\0';
    close(fds[0]);

    int status = 0;
    if (waitpid(pid, &status, 0) < 0) {
        perror("waitpid");
        return 1;
    }
    printf("child wrote: %s", buf);
    if (WIFEXITED(status)) {
        printf("child exit=%d\n", WEXITSTATUS(status));
    } else {
        printf("child did not exit normally\n");
    }
    return 0;
}
