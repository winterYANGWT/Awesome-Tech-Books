// mode: run
// glibc 的 aio_read 用用户态辅助线程模拟：发起后进程线程数从 1 变 2。
#define _GNU_SOURCE
#include <aio.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <time.h>
#include <unistd.h>

static int thread_count(void) {
    FILE *f = fopen("/proc/self/status", "r");
    if (f == NULL) return -1;
    char line[256];
    int n = -1;
    while (fgets(line, sizeof line, f) != NULL) {
        if (strncmp(line, "Threads:", 8) == 0) {
            n = atoi(line + 8);
            break;
        }
    }
    fclose(f);
    return n;
}

int main(void) {
    int sv[2];
    if (socketpair(AF_UNIX, SOCK_STREAM, 0, sv) != 0) {
        perror("socketpair");
        return 1;
    }

    printf("发起 aio_read 前线程数: %d\n", thread_count());

    char byte = 0;
    struct aiocb cb;
    memset(&cb, 0, sizeof cb);
    cb.aio_fildes = sv[0];
    cb.aio_buf = &byte;
    cb.aio_nbytes = 1;
    if (aio_read(&cb) != 0) {
        perror("aio_read");
        return 1;
    }

    // 给辅助线程一点时间创建并阻塞在 read 上。
    struct timespec ts = {0, 100 * 1000 * 1000};
    nanosleep(&ts, NULL);
    printf("发起 aio_read 后线程数: %d\n", thread_count());

    // 写入一个字节让这次读完成，避免辅助线程一直挂着。
    if (write(sv[1], "x", 1) != 1) {
        perror("write");
        return 1;
    }
    struct aiocb *list[1] = {&cb};
    ts.tv_sec = 1;
    ts.tv_nsec = 0;
    if (aio_suspend((const struct aiocb *const *)list, 1, &ts) != 0) {
        perror("aio_suspend");
        return 1;
    }
    ssize_t got = aio_return(&cb);
    printf("aio_read 完成，返回值 %zd\n", got);

    close(sv[0]);
    close(sv[1]);
    return 0;
}
