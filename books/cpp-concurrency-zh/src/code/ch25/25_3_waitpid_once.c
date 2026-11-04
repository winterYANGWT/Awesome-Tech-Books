// mode: hang
// timeout: 2
// expect: (?s)reaped=1.*仍未结束
// 反例：SIGCHLD 处理函数只调用一次 waitpid，其余已退出的子进程留在僵尸态。
#define _POSIX_C_SOURCE 200809L

#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

static volatile sig_atomic_t g_reaped = 0;

static int zombie_count(pid_t *pids, int n) {
    int count = 0;
    for (int i = 0; i < n; ++i) {
        char path[64];
        snprintf(path, sizeof path, "/proc/%ld/stat", (long)pids[i]);
        FILE *f = fopen(path, "r");
        if (f == NULL) {
            continue;
        }
        char buf[512];
        if (fgets(buf, sizeof buf, f) != NULL) {
            char *end = strrchr(buf, ')');
            if (end != NULL && end[1] == ' ' && end[2] == 'Z') {
                ++count;
            }
        }
        fclose(f);
    }
    return count;
}

static void wait_until_zombies(pid_t *pids, int n) {
    struct timespec ts = {0, 1000000L};  // 1 ms
    for (int i = 0; i < 2000; ++i) {
        if (zombie_count(pids, n) == n) {
            return;
        }
        nanosleep(&ts, NULL);
    }
}

static void on_sigchld(int signo) {
    (void)signo;
    int saved = errno;
    int status = 0;
    // 错在这里：只收一个。标准信号不排队，剩下的 SIGCHLD 不会再送来。
    pid_t pid = waitpid(-1, &status, WNOHANG);
    if (pid > 0) {
        ++g_reaped;
        static const char msg[] = "handler: waitpid 只回收了 1 个\n";
        ssize_t ignored = write(STDOUT_FILENO, msg, sizeof msg - 1);
        (void)ignored;
    }
    errno = saved;
}

int main(void) {
    sigset_t chld;
    sigemptyset(&chld);
    sigaddset(&chld, SIGCHLD);
    if (sigprocmask(SIG_BLOCK, &chld, NULL) < 0) {
        perror("sigprocmask");
        return 1;
    }

    struct sigaction sa;
    memset(&sa, 0, sizeof sa);
    sa.sa_handler = on_sigchld;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;
    if (sigaction(SIGCHLD, &sa, NULL) < 0) {
        perror("sigaction");
        return 1;
    }

    pid_t pids[3];
    for (int i = 0; i < 3; ++i) {
        pids[i] = fork();
        if (pids[i] < 0) {
            perror("fork");
            return 1;
        }
        if (pids[i] == 0) {
            _exit(0);
        }
    }

    printf("parent: 3 个子进程已 fork，SIGCHLD 仍被屏蔽\n");
    fflush(stdout);
    wait_until_zombies(pids, 3);
    printf("parent: 僵尸数=%d，现在解除屏蔽\n", zombie_count(pids, 3));
    fflush(stdout);

    sigset_t empty;
    sigemptyset(&empty);
    while (g_reaped == 0) {
        sigsuspend(&empty);  // 原子地临时解除屏蔽，避免检查与 pause 之间的丢信号
    }
    printf("parent: reaped=%d，僵尸数=%d\n", (int)g_reaped,
           zombie_count(pids, 3));
    fflush(stdout);

    for (;;) {
        pause();  // 等不到第 2、3 次 SIGCHLD，卡在这里
    }
}
