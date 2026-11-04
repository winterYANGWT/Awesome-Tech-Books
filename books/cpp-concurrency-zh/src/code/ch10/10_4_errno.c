// mode: run
// expect: 线程 A: open 失败 errno=2 \(ENOENT\)\n线程 B: clock_gettime 失败 errno=22 \(EINVAL\)
#define _POSIX_C_SOURCE 200809L
#include <errno.h>
#include <fcntl.h>
#include <pthread.h>
#include <stdio.h>
#include <time.h>
#include <unistd.h>

typedef struct {
    int err;
} result_t;

static pthread_barrier_t gate;

// ANCHOR: open_worker
static void *open_worker(void *arg) {
    result_t *r = arg;
    pthread_barrier_wait(&gate);  // 让两个线程的失败尽量挨在一起
    int fd = open("/definitely/not/here/ch10", O_RDONLY);
    r->err = errno;  // 失败后立刻读，存进自己的结构体
    if (fd >= 0) close(fd);
    return NULL;
}
// ANCHOR_END: open_worker

static void *clock_worker(void *arg) {
    result_t *r = arg;
    struct timespec ts;
    pthread_barrier_wait(&gate);
    int rc = clock_gettime((clockid_t)-12345, &ts);
    r->err = errno;  // 失败后立刻读，中间不插别的库调用
    (void)rc;
    return NULL;
}

int main(void) {
    if (pthread_barrier_init(&gate, NULL, 2) != 0) return 1;
    result_t a = {0};
    result_t b = {0};
    pthread_t ta;
    pthread_t tb;
    if (pthread_create(&ta, NULL, open_worker, &a) != 0) return 1;
    if (pthread_create(&tb, NULL, clock_worker, &b) != 0) return 1;
    pthread_join(ta, NULL);
    pthread_join(tb, NULL);
    pthread_barrier_destroy(&gate);
    printf("线程 A: open 失败 errno=%d (%s)\n", a.err,
           a.err == ENOENT ? "ENOENT" : "?");
    printf("线程 B: clock_gettime 失败 errno=%d (%s)\n", b.err,
           b.err == EINVAL ? "EINVAL" : "?");
    return 0;
}
