// mode: run
// expect: 信号次数: 5
#define _POSIX_C_SOURCE 200809L
#include <signal.h>
#include <stdio.h>
#include <unistd.h>

// 处理函数只碰这两样东西：volatile sig_atomic_t 和 write。
static volatile sig_atomic_t g_count = 0;

// ANCHOR: handler
static void on_sigusr1(int sig) {
    (void)sig;
    ++g_count;
    const char dot = '.';
    ssize_t n = write(STDOUT_FILENO, &dot, 1);
    (void)n;
}
// ANCHOR_END: handler

int main(void) {
    struct sigaction sa = {0};
    sa.sa_handler = on_sigusr1;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    if (sigaction(SIGUSR1, &sa, NULL) != 0) return 1;

    const int kSignals = 5;
    for (int i = 0; i < kSignals; ++i) {
        if (raise(SIGUSR1) != 0) return 1;
    }
    printf("\n信号次数: %d\n", (int)g_count);
    return 0;
}
