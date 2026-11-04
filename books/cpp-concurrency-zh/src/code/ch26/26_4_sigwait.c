// mode: run
// expect: 定向后：worker 的 pending=1
// expect: sigwait 收到 SIGUSR1=0
// expect: worker 解除屏蔽后：handler=1
// expect: sigwait 收到 SIGTERM=1
#define _POSIX_C_SOURCE 200809L
#include <errno.h>
#include <pthread.h>
#include <signal.h>
#include <stdatomic.h>
#include <stdio.h>
#include <time.h>
#include <unistd.h>

enum { NWORKERS = 4, TARGET = 2 };

static pthread_barrier_t started;
static pthread_barrier_t checked;
static pthread_barrier_t sw_ready;
static pthread_mutex_t gate_mu = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t gate_cv = PTHREAD_COND_INITIALIZER;
static pthread_cond_t allow_cv = PTHREAD_COND_INITIALIZER;
static int release_workers;
static int allow_unblock;
static int target_pending;
static pthread_t workers[NWORKERS];
static atomic_int sw_usr1;
static atomic_int sw_term;
static volatile sig_atomic_t handler_usr1;

static void on_usr1(int signo)
{
    (void)signo;
    handler_usr1 = handler_usr1 + 1;
}

static void *worker(void *arg)
{
    int id = (int)(long)arg;
    sigset_t pending, one;

    pthread_barrier_wait(&started);
    pthread_mutex_lock(&gate_mu);
    while (!release_workers)
        pthread_cond_wait(&gate_cv, &gate_mu);
    pthread_mutex_unlock(&gate_mu);

    if (id == TARGET) {
        sigemptyset(&pending);
        if (sigpending(&pending) == -1)
            return NULL;
        target_pending = sigismember(&pending, SIGUSR1);
        pthread_barrier_wait(&checked);

        pthread_mutex_lock(&gate_mu);
        while (!allow_unblock)
            pthread_cond_wait(&allow_cv, &gate_mu);
        pthread_mutex_unlock(&gate_mu);

        sigemptyset(&one);
        sigaddset(&one, SIGUSR1);
        if (pthread_sigmask(SIG_UNBLOCK, &one, NULL) != 0)
            return NULL;
    }
    return NULL;
}

static void *signal_thread(void *arg)
{
    sigset_t set;
    (void)arg;
    sigemptyset(&set);
    sigaddset(&set, SIGUSR1);
    sigaddset(&set, SIGTERM);
    pthread_barrier_wait(&sw_ready);
    for (;;) {
        int sig = 0;
        int rc = sigwait(&set, &sig);
        if (rc != 0)
            return NULL;
        if (sig == SIGUSR1)
            atomic_fetch_add(&sw_usr1, 1);
        if (sig == SIGTERM) {
            atomic_fetch_add(&sw_term, 1);
            break;
        }
    }
    return NULL;
}

int main(void)
{
    sigset_t mask;
    struct sigaction sa = {0};
    pthread_t sw;

    sigemptyset(&mask);
    sigaddset(&mask, SIGUSR1);
    sigaddset(&mask, SIGTERM);
    sa.sa_handler = on_usr1;
    sigemptyset(&sa.sa_mask);
    if (sigaction(SIGUSR1, &sa, NULL) == -1)
        return 1;
    if (pthread_sigmask(SIG_BLOCK, &mask, NULL) != 0)
        return 1;

    if (pthread_barrier_init(&started, NULL, NWORKERS + 1) != 0 ||
        pthread_barrier_init(&checked, NULL, 2) != 0 ||
        pthread_barrier_init(&sw_ready, NULL, 2) != 0)
        return 1;

    for (long i = 0; i < NWORKERS; ++i) {
        if (pthread_create(&workers[i], NULL, worker, (void *)i) != 0)
            return 1;
    }
    if (pthread_create(&sw, NULL, signal_thread, NULL) != 0)
        return 1;

    pthread_barrier_wait(&started);
    pthread_barrier_wait(&sw_ready);
    if (pthread_kill(workers[TARGET], SIGUSR1) != 0)
        return 1;
    {
        struct timespec ts = {.tv_sec = 0, .tv_nsec = 50000000L};
        nanosleep(&ts, NULL);
    }
    printf("定向后：sigwait 收到 SIGUSR1=%d\n", atomic_load(&sw_usr1));

    pthread_mutex_lock(&gate_mu);
    release_workers = 1;
    pthread_cond_broadcast(&gate_cv);
    pthread_mutex_unlock(&gate_mu);

    pthread_barrier_wait(&checked);
    printf("定向后：worker 的 pending=%d\n", target_pending);
    printf("sigwait 收到 SIGUSR1=%d\n", atomic_load(&sw_usr1));

    pthread_mutex_lock(&gate_mu);
    allow_unblock = 1;
    pthread_cond_signal(&allow_cv);
    pthread_mutex_unlock(&gate_mu);

    for (int i = 0; i < NWORKERS; ++i)
        pthread_join(workers[i], NULL);
    printf("worker 解除屏蔽后：handler=%d\n", (int)handler_usr1);

    (void)kill(getpid(), SIGTERM);
    pthread_join(sw, NULL);
    printf("sigwait 收到 SIGTERM=%d\n", atomic_load(&sw_term));
    return 0;
}
