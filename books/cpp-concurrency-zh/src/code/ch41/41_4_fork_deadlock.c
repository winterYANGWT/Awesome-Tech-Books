// mode: hang
// 主线程持有 log_mutex；工作线程 fork 后，子进程里的 log_line 再锁同一把锁。
#define _POSIX_C_SOURCE 200809L

#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

static pthread_mutex_t log_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t gate_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t gate_cv = PTHREAD_COND_INITIALIZER;
static int gate_open;

static void log_line(const char *text)
{
    size_t len = strlen(text);

    pthread_mutex_lock(&log_mutex);    /* 子进程会停在这里 */
    if (write(STDOUT_FILENO, text, len) < 0) {
        /* 日志失败也不能在持锁时退出 */
    }
    pthread_mutex_unlock(&log_mutex);
}

static void *worker(void *arg)
{
    pid_t pid;

    (void)arg;
    pthread_mutex_lock(&gate_mutex);
    while (!gate_open) {
        pthread_cond_wait(&gate_cv, &gate_mutex);
    }
    pthread_mutex_unlock(&gate_mutex);

    pid = fork();
    if (pid == 0) {
        /* write 不经 stdio，也不碰 log_mutex，先说明自己走到了哪一步 */
        (void)!write(STDOUT_FILENO, "child: calling log_line (needs log_mutex)\n", 42);
        log_line("child: got the lock (never printed)\n");
        _exit(0);
    }
    if (pid == -1) {
        perror("fork");
        return NULL;
    }
    (void)!write(STDOUT_FILENO, "parent worker: forked, waiting for child\n", 41);
    waitpid(pid, NULL, 0);             /* 子进程死锁，这里永远不返回 */
    return NULL;
}

int main(void)
{
    pthread_t tid;

    pthread_mutex_lock(&log_mutex);    /* 主线程故意一直持有日志锁 */
    if (pthread_create(&tid, NULL, worker, NULL) != 0) {
        return 1;
    }

    pthread_mutex_lock(&gate_mutex);
    gate_open = 1;
    pthread_cond_signal(&gate_cv);
    pthread_mutex_unlock(&gate_mutex);

    pthread_join(tid, NULL);           /* 工作线程等子进程，主线程等工作线程 */
    return 0;
}
