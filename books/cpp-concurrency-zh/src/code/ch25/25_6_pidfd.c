// pidfd_open + poll + waitid(P_PIDFD)：用不会复用的句柄等子进程退出。
#define _GNU_SOURCE

#include <errno.h>
#include <poll.h>
#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <sys/syscall.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

int main(void) {
    pid_t pid = fork();
    if (pid < 0) {
        perror("fork");
        return 1;
    }
    if (pid == 0) {
        struct timespec ts = {0, 100 * 1000 * 1000L};  // 100 ms
        nanosleep(&ts, NULL);
        _exit(7);
    }

    int pidfd = (int)syscall(SYS_pidfd_open, pid, 0);
    if (pidfd < 0) {
        perror("pidfd_open");
        return 1;
    }

    struct pollfd pfd;
    pfd.fd = pidfd;
    pfd.events = POLLIN;
    pfd.revents = 0;
    int rc;
    do {
        rc = poll(&pfd, 1, -1);
    } while (rc < 0 && errno == EINTR);
    if (rc < 0) {
        perror("poll");
        close(pidfd);
        return 1;
    }
    printf("poll: pidfd 可读，进程已经退出\n");

    siginfo_t info;
    memset(&info, 0, sizeof info);
    if (waitid(P_PIDFD, (id_t)pidfd, &info, WEXITED) < 0) {
        perror("waitid");
        close(pidfd);
        return 1;
    }
    printf("waitid: si_pid 匹配=%d si_status=%d\n",
           info.si_pid == pid ? 1 : 0, info.si_status);
    close(pidfd);
    return 0;
}
