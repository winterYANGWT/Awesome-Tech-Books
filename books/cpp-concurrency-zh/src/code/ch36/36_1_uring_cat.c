// mode: run
// flags: -luring
// expect: bytes=26
#define _GNU_SOURCE
#include <fcntl.h>
#include <liburing.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

// 读一个临时文件，再用 io_uring 写到 stdout。两次操作都走完成队列。
static const char payload[] = "completion, not readiness\n";

int main(void) {
    char path[] = "/tmp/uring_cat_XXXXXX";
    int file = mkstemp(path);
    if (file < 0) { perror("mkstemp"); return 1; }
    unlink(path);
    if (write(file, payload, sizeof payload - 1) != (ssize_t)(sizeof payload - 1)) {
        perror("write");
        return 1;
    }

    struct io_uring ring;
    int ret = io_uring_queue_init(8, &ring, 0);
    if (ret < 0) {
        fprintf(stderr, "io_uring_queue_init: %s\n", strerror(-ret));
        return 1;
    }

    char buf[64];
    struct io_uring_sqe *sqe = io_uring_get_sqe(&ring);
    io_uring_prep_read(sqe, file, buf, sizeof buf, 0);
    io_uring_sqe_set_data64(sqe, 1);
    int submitted = io_uring_submit(&ring);

    struct io_uring_cqe *cqe;
    ret = io_uring_wait_cqe(&ring, &cqe);
    if (ret < 0) {
        fprintf(stderr, "wait_cqe: %s\n", strerror(-ret));
        return 1;
    }
    ssize_t nread = cqe->res;
    fprintf(stderr, "read: submitted=%d res=%zd\n", submitted, nread);
    io_uring_cqe_seen(&ring, cqe);
    if (nread <= 0) return 1;

    // 写到 fd 1，同样先取 SQE、填、提交、等完成。
    sqe = io_uring_get_sqe(&ring);
    io_uring_prep_write(sqe, STDOUT_FILENO, buf, (unsigned)nread, 0);
    io_uring_sqe_set_data64(sqe, 2);
    submitted = io_uring_submit(&ring);
    ret = io_uring_wait_cqe(&ring, &cqe);
    if (ret < 0) {
        fprintf(stderr, "wait_cqe: %s\n", strerror(-ret));
        return 1;
    }
    ssize_t nwritten = cqe->res;
    io_uring_cqe_seen(&ring, cqe);
    fprintf(stderr, "write: submitted=%d res=%zd bytes=%zd\n", submitted,
            nwritten, nread);

    close(file);
    io_uring_queue_exit(&ring);
    return nwritten == nread ? 0 : 1;
}
