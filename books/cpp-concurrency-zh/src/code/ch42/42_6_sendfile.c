// mode: run
// expect: sent=100000, received=100000, mismatches=0
// 文件到 socket 用 sendfile：数据不经过用户态缓冲，接收端逐字节校验。
#define _GNU_SOURCE
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/sendfile.h>
#include <sys/socket.h>
#include <unistd.h>

#define TOTAL 100000

int main(void) {
    char path[] = "/tmp/ch42_sendfile_XXXXXX";
    int file = mkstemp(path);
    if (file < 0) {
        perror("mkstemp");
        return 1;
    }
    unlink(path);

    unsigned char *src = malloc(TOTAL);
    if (!src) {
        return 1;
    }
    for (int i = 0; i < TOTAL; i++) {
        src[i] = (unsigned char)(i % 251);
    }
    if (write(file, src, TOTAL) != TOTAL) {
        perror("write");
        return 1;
    }
    lseek(file, 0, SEEK_SET);

    int sv[2];
    if (socketpair(AF_UNIX, SOCK_STREAM, 0, sv) != 0) {
        perror("socketpair");
        return 1;
    }

    off_t offset = 0;
    size_t sent = 0;
    while (sent < TOTAL) {
        ssize_t n = sendfile(sv[0], file, &offset, TOTAL - sent);
        if (n < 0) {
            perror("sendfile");
            return 1;
        }
        if (n == 0) {
            break;
        }
        sent += (size_t)n;
    }

    unsigned char *dst = malloc(TOTAL);
    if (!dst) {
        return 1;
    }
    size_t received = 0;
    while (received < TOTAL) {
        ssize_t n = read(sv[1], dst + received, TOTAL - received);
        if (n <= 0) {
            break;
        }
        received += (size_t)n;
    }

    size_t mismatches = 0;
    for (size_t i = 0; i < received; i++) {
        if (dst[i] != src[i]) {
            mismatches++;
        }
    }
    printf("sent=%zu, received=%zu, mismatches=%zu\n", sent, received, mismatches);

    close(sv[0]);
    close(sv[1]);
    close(file);
    free(src);
    free(dst);
    return 0;
}
