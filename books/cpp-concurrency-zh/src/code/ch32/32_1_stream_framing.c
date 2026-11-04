// mode: run
// TCP 是字节流：读几次不等于有几个请求。socketpair 在本机模拟一条双向字节流。
#define _GNU_SOURCE
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

static void write_all(int fd, const char *p, size_t n) {
    while (n > 0) {
        ssize_t k = write(fd, p, n);
        if (k < 0) {
            if (errno == EINTR) continue;
            perror("write");
            _exit(1);
        }
        p += k;
        n -= (size_t)k;
    }
}

int main(void) {
    int sv[2];
    if (socketpair(AF_UNIX, SOCK_STREAM, 0, sv) != 0) {
        perror("socketpair");
        return 1;
    }

    // 发送端写三次，每次 16 字节；接收端每次只要 16 字节。
    for (int i = 0; i < 3; ++i) {
        char block[16];
        memset(block, 'A' + i, sizeof block);
        write_all(sv[0], block, sizeof block);
        char buf[16];
        ssize_t n = read(sv[1], buf, sizeof buf);
        printf("读 %d: %zd 字节\n", i + 1, n);
    }

    // 两条完整请求一次写入，接收端一次就把两条都读走。
    const char two[] = "GET alpha\nGET beta\n";
    write_all(sv[0], two, sizeof two - 1);
    char buf[64];
    ssize_t n = read(sv[1], buf, sizeof buf);
    int lines = 0;
    for (ssize_t i = 0; i < n; ++i)
        if (buf[i] == '\n') ++lines;
    printf("一次读到 %zd 字节，其中 %d 个 '\\n'\n", n, lines);

    close(sv[0]);
    close(sv[1]);
    return 0;
}
