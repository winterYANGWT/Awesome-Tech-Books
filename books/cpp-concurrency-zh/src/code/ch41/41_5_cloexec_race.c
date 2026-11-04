// mode: run
// open 之后再用 fcntl 设 FD_CLOEXEC 有窗口；O_CLOEXEC 在 open 时原子设好。
#define _GNU_SOURCE

#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <unistd.h>

/* fork + exec 一个 shell，让它检查 fd 是否出现在自己进程的 /proc/self/fd 里 */
static int child_sees(int fd)
{
    char cmd[64];
    pid_t pid = fork();
    int status = 0;

    if (pid == 0) {
        snprintf(cmd, sizeof cmd, "test -e /proc/self/fd/%d", fd);
        execl("/bin/sh", "sh", "-c", cmd, (char *)NULL);
        _exit(127);
    }
    if (pid == -1) {
        perror("fork");
        exit(1);
    }
    if (waitpid(pid, &status, 0) != pid) {
        perror("waitpid");
        exit(1);
    }
    return WIFEXITED(status) && WEXITSTATUS(status) == 0;
}

static int open_hostname(int flags)
{
    int fd = open("/etc/hostname", O_RDONLY | flags);

    if (fd == -1) {
        perror("open");
        exit(1);
    }
    return fd;
}

int main(void)
{
    int fd = open_hostname(0);

    printf("round 1: fd=%d\n", fd);
    printf("round 1, before fcntl: child sees fd: %s\n",
           child_sees(fd) ? "yes" : "no");

    if (fcntl(fd, F_SETFD, FD_CLOEXEC) == -1) {
        perror("fcntl");
        return 1;
    }
    printf("round 1, after fcntl: child sees fd: %s\n",
           child_sees(fd) ? "yes" : "no");
    close(fd);

    fd = open_hostname(O_CLOEXEC);
    printf("round 2, O_CLOEXEC: child sees fd: %s\n",
           child_sees(fd) ? "yes" : "no");
    close(fd);
    return 0;
}
