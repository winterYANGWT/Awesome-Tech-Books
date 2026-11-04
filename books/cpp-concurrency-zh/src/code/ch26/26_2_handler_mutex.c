// mode: hang
// timeout: 2
// expect: 主线程已持有 mutex，准备 raise(SIGUSR1)
// expect: 仍未结束
#define _POSIX_C_SOURCE 200809L
#include <pthread.h>
#include <signal.h>
#include <stdio.h>

static pthread_mutex_t mu;

static void on_usr1(int signo)
{
    (void)signo;
    /* 普通互斥量同线程重入：POSIX 允许实现直接死锁。 */
    (void)pthread_mutex_lock(&mu);
}

int main(void)
{
    struct sigaction sa = {0};
    pthread_mutexattr_t attr;

    if (pthread_mutexattr_init(&attr) != 0 ||
        pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_NORMAL) != 0 ||
        pthread_mutex_init(&mu, &attr) != 0)
        return 1;

    sa.sa_handler = on_usr1;
    sigemptyset(&sa.sa_mask);
    if (sigaction(SIGUSR1, &sa, NULL) == -1)
        return 1;

    if (pthread_mutex_lock(&mu) != 0)
        return 1;
    printf("主线程已持有 mutex，准备 raise(SIGUSR1)\n");
    fflush(stdout);
    raise(SIGUSR1);

    /* 正常情况下走不到这里。 */
    (void)pthread_mutex_unlock(&mu);
    printf("没有卡住：互斥量允许了同线程重入\n");
    return 0;
}
