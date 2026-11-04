// mode: run
// expect: raise 两次：raised=2
// expect: 解除屏蔽前：pending=1 handled=0
// expect: 解除屏蔽后：handled=1
#define _POSIX_C_SOURCE 200809L
#include <errno.h>
#include <signal.h>
#include <stdio.h>

static volatile sig_atomic_t handled;

static void on_usr1(int signo)
{
    (void)signo;
    handled = handled + 1;
}

int main(void)
{
    struct sigaction sa = {0};
    sigset_t block, old, pending;

    sa.sa_handler = on_usr1;
    sigemptyset(&sa.sa_mask);
    if (sigaction(SIGUSR1, &sa, NULL) == -1) {
        perror("sigaction");
        return 1;
    }

    sigemptyset(&block);
    sigaddset(&block, SIGUSR1);
    if (sigprocmask(SIG_BLOCK, &block, &old) == -1) {
        perror("sigprocmask");
        return 1;
    }

    if (raise(SIGUSR1) != 0 || raise(SIGUSR1) != 0) {
        perror("raise");
        return 1;
    }
    printf("raise 两次：raised=2\n");

    if (sigpending(&pending) == -1) {
        perror("sigpending");
        return 1;
    }
    printf("解除屏蔽前：pending=%d handled=%d\n",
           sigismember(&pending, SIGUSR1), (int)handled);

    if (sigprocmask(SIG_SETMASK, &old, NULL) == -1) {
        perror("sigprocmask");
        return 1;
    }
    printf("解除屏蔽后：handled=%d\n", (int)handled);
    return 0;
}
