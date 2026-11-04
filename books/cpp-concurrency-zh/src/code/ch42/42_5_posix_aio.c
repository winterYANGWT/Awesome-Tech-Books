// mode: run
// expect: threads before aio_read = 1
// expect: threads after aio_read = 2
// POSIX AIO 的 aio_read 在 glibc 里由用户态辅助线程完成；数 /proc/self/task 能看到它。
#define _GNU_SOURCE
#include <aio.h>
#include <dirent.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

static int thread_count(void) {
    DIR *d = opendir("/proc/self/task");
    if (!d) {
        perror("opendir");
        exit(1);
    }
    int n = 0;
    struct dirent *e;
    while ((e = readdir(d)) != NULL) {
        if (e->d_name[0] != '.') {
            n++;
        }
    }
    closedir(d);
    return n;
}

int main(void) {
    char path[] = "/tmp/ch42_aio_XXXXXX";
    int fd = mkstemp(path);
    if (fd < 0) {
        perror("mkstemp");
        return 1;
    }
    unlink(path);
    const char text[] = "alpha beta\n";
    if (write(fd, text, sizeof text - 1) != (ssize_t)(sizeof text - 1)) {
        perror("write");
        return 1;
    }
    lseek(fd, 0, SEEK_SET);

    printf("threads before aio_read = %d\n", thread_count());

    char buf[64] = {0};
    struct aiocb cb;
    memset(&cb, 0, sizeof cb);
    cb.aio_fildes = fd;
    cb.aio_buf = buf;
    cb.aio_nbytes = sizeof buf - 1;
    cb.aio_offset = 0;

    int ret = aio_read(&cb);
    printf("aio_read = %d\n", ret);
    printf("threads after aio_read = %d\n", thread_count());

    const struct aiocb *list[1] = {&cb};
    ret = aio_suspend(list, 1, NULL);
    ssize_t done = aio_return(&cb);
    printf("aio_suspend = %d, aio_return = %zd\n", ret, done);
    printf("aio_error = %d\n", aio_error(&cb));
    printf("data = %s", buf);
    close(fd);
    return 0;
}
