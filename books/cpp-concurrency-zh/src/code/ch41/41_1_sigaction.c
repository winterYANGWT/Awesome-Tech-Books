// mode: run
// 用 SA_SIGINFO 取 si_pid / si_code，处理函数只写 sig_atomic_t 和 pthread_t。
#define _POSIX_C_SOURCE 200809L

#include <errno.h>
#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

static volatile sig_atomic_t got;
static volatile sig_atomic_t sender_pid;
static volatile sig_atomic_t sender_code;
static pthread_t handler_thread;
static pthread_t main_thread;

static void on_usr1(int signo, siginfo_t *si, void *ctx)
{
    (void)ctx;
    (void)signo;
    handler_thread = pthread_self();   /* pthread_self 在 async-signal-safe 列表里 */
    sender_pid = (sig_atomic_t)si->si_pid;
    sender_code = (sig_atomic_t)si->si_code;
    got = 1;
}

int main(void)
{
    struct sigaction sa;
    memset(&sa, 0, sizeof sa);
    sa.sa_sigaction = on_usr1;
    sa.sa_flags = SA_SIGINFO;          /* 故意不设 SA_RESTART */
    sigemptyset(&sa.sa_mask);

    if (sigaction(SIGUSR1, &sa, NULL) == -1) {
        perror("sigaction");
        return 1;
    }

    main_thread = pthread_self();
    if (kill(getpid(), SIGUSR1) == -1) {
        perror("kill");
        return 1;
    }

    while (!got) {
        pause();                       /* 处理函数返回后 pause 返回 EINTR */
    }

    printf("handler ran: %s\n", got ? "yes" : "no");
    printf("handler on main thread: %s\n",
           pthread_equal(handler_thread, main_thread) ? "yes" : "no");
    printf("si_pid is our pid: %s\n",
           sender_pid == (sig_atomic_t)getpid() ? "yes" : "no");
    printf("si_code is SI_USER: %s\n", sender_code == SI_USER ? "yes" : "no");
    return 0;
}
