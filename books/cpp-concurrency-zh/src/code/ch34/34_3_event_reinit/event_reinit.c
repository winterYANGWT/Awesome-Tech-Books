// fork 之后 event_reinit 的作用：子进程要拿到自己的后端实例。
// 这个例子用 ET 管道放大差别：不调用 event_reinit 时，父子共享同一个 epoll
// 实例，父进程会先把这条边沿取走，子进程的读事件永远不触发。
#define _POSIX_C_SOURCE 200809L
#include <event2/event.h>

#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

static struct event_base *base;
static pid_t parent_pid;

struct ReadCtx {
    int fd;
    int reads;
    const char *mode;
};

static const char *me(void) {
    return getpid() == parent_pid ? "parent" : "child";
}

static void on_read(evutil_socket_t fd, short what, void *arg) {
    (void)what;
    struct ReadCtx *c = (struct ReadCtx *)arg;
    char b;
    ssize_t n = read(fd, &b, 1);
    if (n == 1) ++c->reads;
    printf("%s: ET read fired (reads=%d)\n", me(), c->reads);
    fflush(stdout);
    event_base_loopexit(base, NULL);
}

static void on_write_timer(evutil_socket_t fd, short what, void *arg) {
    (void)fd;
    (void)what;
    int out = *(int *)arg;
    if (write(out, "x", 1) != 1) perror("write");
    if (getpid() == parent_pid) {
        printf("parent: write timer fired\n");
    } else {
        printf("child: pre-fork write timer fired\n");
    }
    fflush(stdout);
}

static void on_stop(evutil_socket_t fd, short what, void *arg) {
    (void)fd;
    (void)what;
    (void)arg;
    event_base_loopexit(base, NULL);
}

int main(int argc, char **argv) {
    int do_reinit = !(argc > 1 && strcmp(argv[1], "--no-reinit") == 0);
    parent_pid = getpid();

    int p[2];
    if (pipe(p) != 0) return 1;
    if (fcntl(p[0], F_SETFL, O_NONBLOCK) != 0) return 1;

    base = event_base_new();
    if (base == NULL) return 1;

    struct event *wt = evtimer_new(base, on_write_timer, &p[1]);
    struct timeval wtv = {0, 100000};
    event_add(wt, &wtv);

    struct event *pt = evtimer_new(base, on_stop, NULL);
    struct timeval ptv = {0, 700000};
    event_add(pt, &ptv);

    pid_t pid = fork();
    if (pid == 0) {
        // 子进程不需要父进程留下的写定时器和退出定时器，先删掉，
        // 否则子进程会再写一个字节，制造出新的边沿，掩盖共享 epoll 的问题。
        event_del(wt);
        event_del(pt);
        if (do_reinit) {
            printf("child: event_reinit=%d\n", event_reinit(base));
        } else {
            printf("child: event_reinit skipped\n");
        }

        struct ReadCtx rc = {p[0], 0, do_reinit ? "reinit" : "no-reinit"};
        struct event *rev = event_new(base, p[0], EV_READ | EV_ET | EV_PERSIST,
                                      on_read, &rc);
        event_add(rev, NULL);

        struct event *ct = evtimer_new(base, on_stop, NULL);
        struct timeval ctv = {0, 500000};
        event_add(ct, &ctv);

        usleep(300000);  // 让父进程先成为唯一的 epoll_wait 等待者
        int rc_loop = event_base_dispatch(base);
        printf("child: dispatch=%d reads=%d\n", rc_loop, rc.reads);
        fflush(stdout);
        _exit(0);
    }

    // 父进程在 300 ms 内不断非阻塞地跑循环，确保它先把共享 epoll 上的边沿取走。
    struct timespec start;
    if (clock_gettime(CLOCK_MONOTONIC, &start) != 0) return 1;
    for (;;) {
        struct timespec now;
        if (clock_gettime(CLOCK_MONOTONIC, &now) != 0) return 1;
        long elapsed_ms = (now.tv_sec - start.tv_sec) * 1000 +
                          (now.tv_nsec - start.tv_nsec) / 1000000;
        if (elapsed_ms >= 300) break;
        event_base_loop(base, EVLOOP_NONBLOCK);
        usleep(1000);
    }

    int rc = event_base_dispatch(base);
    printf("parent: dispatch=%d\n", rc);
    int status = 0;
    waitpid(pid, &status, 0);
    printf("child: exit=%d\n", WIFEXITED(status) ? WEXITSTATUS(status) : -1);
    return 0;
}
