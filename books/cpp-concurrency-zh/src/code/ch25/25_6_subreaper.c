// PR_SET_CHILD_SUBREAPER：子进程先退出后，孙进程被当前进程收养，waitpid 能收到它。
#define _GNU_SOURCE

#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <sys/prctl.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

static void sleep_ms(long ms) {
    struct timespec ts;
    ts.tv_sec = ms / 1000;
    ts.tv_nsec = (ms % 1000) * 1000000L;
    nanosleep(&ts, NULL);
}

int main(void) {
    if (prctl(PR_SET_CHILD_SUBREAPER, 1) < 0) {
        perror("prctl");
        return 1;
    }
    printf("subreaper: on\n");

    int fds[2];
    if (pipe(fds) < 0) {
        perror("pipe");
        return 1;
    }

    pid_t child = fork();
    if (child < 0) {
        perror("fork");
        return 1;
    }
    if (child == 0) {
        close(fds[0]);
        pid_t grandchild = fork();
        if (grandchild < 0) {
            _exit(1);
        }
        if (grandchild == 0) {
            close(fds[1]);
            sleep_ms(100);
            _exit(5);
        }
        ssize_t ignored = write(fds[1], &grandchild, sizeof grandchild);
        (void)ignored;
        close(fds[1]);
        _exit(0);  // 子进程先退出，孙进程变成孤儿
    }

    close(fds[1]);
    pid_t grandchild = -1;
    ssize_t n;
    do {
        n = read(fds[0], &grandchild, sizeof grandchild);
    } while (n < 0 && errno == EINTR);
    close(fds[0]);

    int status = 0;
    if (waitpid(child, &status, 0) < 0) {
        perror("waitpid child");
        return 1;
    }
    printf("child: exit=%d\n", WEXITSTATUS(status));

    pid_t got = waitpid(grandchild, &status, 0);
    if (got < 0) {
        perror("waitpid grandchild");
        return 1;
    }
    printf("grandchild: 被 subreaper 收养, exit=%d\n", WEXITSTATUS(status));
    return 0;
}
