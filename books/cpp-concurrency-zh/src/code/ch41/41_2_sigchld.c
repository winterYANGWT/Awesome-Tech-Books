// mode: run
// SIGCHLD 的处理函数只设标志；主循环收到标志后用 waitpid(WNOHANG) 回收，返回 0 表示还有子进程在跑。
#define _GNU_SOURCE

#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

static volatile sig_atomic_t chld_arrived;

static void on_chld(int signo)
{
    (void)signo;
    chld_arrived = 1;
}

int main(void)
{
    sigset_t block, oldmask;
    struct sigaction sa;
    int reaped = 0;

    sigemptyset(&block);
    sigaddset(&block, SIGCHLD);
    /* 先屏蔽，保证“检查标志”和 sigsuspend 之间信号不会丢 */
    if (sigprocmask(SIG_BLOCK, &block, &oldmask) == -1) {
        perror("sigprocmask");
        return 1;
    }

    memset(&sa, 0, sizeof sa);
    sa.sa_handler = on_chld;
    sigemptyset(&sa.sa_mask);
    if (sigaction(SIGCHLD, &sa, NULL) == -1) {
        perror("sigaction");
        return 1;
    }

    for (int i = 0; i < 3; i++) {
        pid_t pid = fork();
        if (pid == 0) {
            _exit(0);                  /* 子进程不刷新 stdio，避免重复输出 */
        }
        if (pid == -1) {
            perror("fork");
            return 1;
        }
    }

    while (reaped < 3) {
        while (chld_arrived) {
            pid_t pid;

            chld_arrived = 0;
            while ((pid = waitpid(-1, NULL, WNOHANG)) > 0) {
                reaped++;
            }
            if (pid == 0) {
                break;                 /* 还有子进程在跑，等下一次通知 */
            }
        }
        if (reaped < 3 && sigsuspend(&oldmask) == -1 && errno != EINTR) {
            perror("sigsuspend");
            break;
        }
    }

    printf("reaped=%d\n", reaped);
    errno = 0;
    {
        pid_t pid = waitpid(-1, NULL, WNOHANG);
        printf("extra waitpid returned %zd, ECHILD: %s\n",
               (ssize_t)pid, (pid == -1 && errno == ECHILD) ? "yes" : "no");
    }
    return 0;
}
