#define _POSIX_C_SOURCE 200809L
#include <errno.h>
#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

static pthread_mutex_t mu = PTHREAD_MUTEX_INITIALIZER;
static int ready_fd = -1;

static void on_alarm(int signo)
{
    (void)signo; /* gdb 在这里停下，栈里能看到下面卡住的 lock。 */
}

static void sleep_seconds(long sec)
{
    struct timespec ts = {.tv_sec = sec, .tv_nsec = 0};
    while (nanosleep(&ts, &ts) == -1 && errno == EINTR)
        ;
}

static void *worker(void *arg)
{
    (void)arg;
    if (pthread_mutex_lock(&mu) != 0)
        return NULL;
    printf("worker：已持有 mutex，开始睡眠\n");
    fflush(stdout);
    char b = 1;
    ssize_t n = write(ready_fd, &b, 1);
    (void)n;
    sleep_seconds(5);
    (void)pthread_mutex_unlock(&mu);
    return NULL;
}

int main(void)
{
    int p[2];
    pthread_t th;
    if (pipe(p) == -1)
        return 1;
    ready_fd = p[1];
    if (pthread_create(&th, NULL, worker, NULL) != 0)
        return 1;
    char b;
    if (read(p[0], &b, 1) != 1)
        return 1;

    printf("父进程：mutex 已被 worker 持有，现在 fork\n");
    fflush(stdout);
    pid_t child = fork();
    if (child == -1)
        return 1;
    if (child == 0) {
        signal(SIGALRM, on_alarm);
        alarm(2);
        printf("子进程：准备对同一把锁加锁\n");
        fflush(stdout);
        (void)pthread_mutex_lock(&mu);
        printf("子进程：居然拿到了锁\n");
        _exit(0);
    }

    int status = 0;
    if (waitpid(child, &status, 0) == -1)
        return 1;
    pthread_join(th, NULL);
    return 0;
}
