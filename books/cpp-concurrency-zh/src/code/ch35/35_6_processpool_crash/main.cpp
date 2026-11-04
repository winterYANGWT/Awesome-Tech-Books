// 子进程被 kill -9 后，父进程用 SIGCHLD 回收并补上一个。
// mode: run
// expect: 进程数从 3 回到 4
#include <pthread.h>
#include <signal.h>
#include <sys/wait.h>
#include <unistd.h>

#include <cstdio>
#include <cstdlib>

int main() {
    // 先屏蔽 SIGCHLD，后面用 sigwait 同步等，避免信号在检查与 suspend 之间丢失。
    sigset_t mask;
    sigemptyset(&mask);
    sigaddset(&mask, SIGCHLD);
    pthread_sigmask(SIG_BLOCK, &mask, nullptr);

    pid_t children[4] = {};
    for (int i = 0; i < 4; ++i) {
        const pid_t pid = fork();
        if (pid < 0) return 1;
        if (pid == 0) {
            for (;;) pause();
        }
        children[i] = pid;
    }
    std::printf("启动 4 个子进程\n");

    // 杀掉一个，模拟子进程崩溃。
    kill(children[2], SIGKILL);
    int signo = 0;
    sigwait(&mask, &signo);

    int status = 0;
    const pid_t reaped = waitpid(-1, &status, 0);
    if (reaped < 0) return 1;
    std::printf("回收子进程，存活 3 个\n");
    children[2] = -1;

    // 补一个，仍占原来的槽位。
    const pid_t replacement = fork();
    if (replacement < 0) return 1;
    if (replacement == 0) {
        for (;;) pause();
    }
    children[2] = replacement;
    std::printf("进程数从 3 回到 4\n");

    for (int i = 0; i < 4; ++i) kill(children[i], SIGTERM);
    for (int i = 0; i < 4; ++i) {
        if (children[i] > 0) waitpid(children[i], nullptr, 0);
    }
    std::printf("收到 SIGTERM，4 个子进程全部回收\n");
}
