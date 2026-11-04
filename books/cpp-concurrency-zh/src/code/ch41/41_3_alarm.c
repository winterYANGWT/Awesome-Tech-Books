// mode: run
// alarm 只触发一次：不重新 alarm，再过一段时间计数也不变。
#define _XOPEN_SOURCE 700

#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

static volatile sig_atomic_t fired;

static void on_alarm(int signo)
{
    (void)signo;
    fired++;
}

static void nap_ms(long ms)
{
    struct timespec ts;

    ts.tv_sec = ms / 1000;
    ts.tv_nsec = (ms % 1000) * 1000000L;
    while (nanosleep(&ts, &ts) == -1 && errno == EINTR) {
    }
}

int main(void)
{
    struct sigaction sa;

    memset(&sa, 0, sizeof sa);
    sa.sa_handler = on_alarm;
    sigemptyset(&sa.sa_mask);
    if (sigaction(SIGALRM, &sa, NULL) == -1) {
        perror("sigaction");
        return 1;
    }

    alarm(1);
    nap_ms(1200);
    printf("after 1.2s: fired=%d\n", (int)fired);

    nap_ms(1200);
    printf("after 2.4s, no re-arm: fired=%d\n", (int)fired);

    alarm(1);
    nap_ms(1200);
    printf("after re-arm: fired=%d\n", (int)fired);
    return 0;
}
