// mode: run
// output: varies
// expect: total written = 131072
// expect: remaining = 0
// 非阻塞管道的 writev 只写一部分：必须按返回值推进 iov，而不是当成失败重来。
#define _GNU_SOURCE
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/uio.h>
#include <unistd.h>

#define CHUNK 32768
#define NIOV 4

// 跳过已经写完的 iov，部分写的那个调整 base 和 len。
static void advance(struct iovec **iov, int *cnt, size_t n) {
    while (n > 0 && *cnt > 0) {
        if (n >= (*iov)->iov_len) {
            n -= (*iov)->iov_len;
            (*iov)++;
            (*cnt)--;
        } else {
            (*iov)->iov_base = (char *)(*iov)->iov_base + n;
            (*iov)->iov_len -= n;
            n = 0;
        }
    }
}

static void drain(int fd) {
    char buf[65536];
    while (read(fd, buf, sizeof buf) > 0) {
    }
}

int main(void) {
    int p[2];
    if (pipe2(p, O_NONBLOCK | O_CLOEXEC) != 0) {
        perror("pipe2");
        return 1;
    }
    // 默认管道容量因内核而异；调小一点，让部分写稳定出现。
    if (fcntl(p[1], F_SETPIPE_SZ, 8192) < 0) {
        perror("F_SETPIPE_SZ");
    }
    int cap = fcntl(p[1], F_GETPIPE_SZ);

    char *buf[NIOV];
    struct iovec iov[NIOV];
    for (int i = 0; i < NIOV; i++) {
        buf[i] = malloc(CHUNK);
        if (!buf[i]) {
            return 1;
        }
        memset(buf[i], 'A' + i, CHUNK);
        iov[i].iov_base = buf[i];
        iov[i].iov_len = CHUNK;
    }

    size_t total = (size_t)CHUNK * NIOV;
    size_t written = 0;
    size_t first = 0;
    int calls = 0;
    struct iovec *cur = iov;
    int cnt = NIOV;

    while (written < total) {
        ssize_t n = writev(p[1], cur, cnt);
        calls++;
        if (n < 0) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                drain(p[0]);  // 读走一部分，腾出空间再写
                continue;
            }
            perror("writev");
            return 1;
        }
        if (first == 0) {
            first = (size_t)n;
        }
        written += (size_t)n;
        advance(&cur, &cnt, (size_t)n);
    }

    printf("pipe capacity = %d, total = %zu\n", cap, total);
    printf("first writev = %zu (partial, not an error)\n", first);
    printf("writev calls = %d, iov left = %d\n", calls, cnt);
    printf("total written = %zu\n", written);
    printf("remaining = %zu\n", total - written);

    for (int i = 0; i < NIOV; i++) {
        free(buf[i]);
    }
    close(p[0]);
    close(p[1]);
    return 0;
}
