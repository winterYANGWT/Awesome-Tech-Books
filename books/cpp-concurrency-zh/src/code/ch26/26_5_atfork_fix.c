// mode: run
// tsan: no（fork 后子进程只剩调用线程，TSan 把未复制的 worker 报成 thread leak）
// expect: 父进程：准备 fork，prepare 会等 worker 放锁
// expect: 子进程：加锁成功，准备 _exit
// expect: 父进程：回收成功
#define _POSIX_C_SOURCE 200809L
#include <errno.h>
#include <pthread.h>
#include <stdio.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

static pthread_mutex_t mu = PTHREAD_MUTEX_INITIALIZER;
static int ready_fd = -1;

static void sleep_ms(long ms)
{
    struct timespec ts = {.tv_sec = ms / 1000,
                          .tv_nsec = (ms % 1000) * 1000000L};
    while (nanosleep(&ts, &ts) == -1 && errno == EINTR)
        ;
}

static void *worker(void *arg)
{
    (void)arg;
    if (pthread_mutex_lock(&mu) != 0)
        return NULL;
    char b = 1;
    ssize_t n = write(ready_fd, &b, 1);
    (void)n;
    sleep_ms(200);
    (void)pthread_mutex_unlock(&mu);
    return NULL;
}

static void prepare(void)
{
    (void)pthread_mutex_lock(&mu);
}

static void parent_side(void)
{
    (void)pthread_mutex_unlock(&mu);
}

static void child_side(void)
{
    (void)pthread_mutex_unlock(&mu);
}

int main(void)
{
    int p[2];
    pthread_t th;
    if (pipe(p) == -1)
        return 1;
    ready_fd = p[1];
    if (pthread_atfork(prepare, parent_side, child_side) != 0)
        return 1;
    if (pthread_create(&th, NULL, worker, NULL) != 0)
        return 1;

    char b;
    if (read(p[0], &b, 1) != 1)
        return 1;
    printf("父进程：准备 fork，prepare 会等 worker 放锁\n");
    fflush(stdout);

    pid_t child = fork();
    if (child == -1)
        return 1;
    if (child == 0) {
        if (pthread_mutex_lock(&mu) != 0)
            _exit(2);
        (void)pthread_mutex_unlock(&mu);
        printf("子进程：加锁成功，准备 _exit\n");
        fflush(stdout);
        _exit(0);
    }

    int status = 0;
    if (waitpid(child, &status, 0) == -1)
        return 1;
    pthread_join(th, NULL);
    if (WIFEXITED(status) && WEXITSTATUS(status) == 0)
        printf("父进程：回收成功\n");
    else
        printf("父进程：子进程退出异常\n");
    return 0;
}
