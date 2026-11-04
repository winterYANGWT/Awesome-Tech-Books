// mode: run
// flags: -luring
// expect: first get_sqe: NULL
#define _GNU_SOURCE
#include <liburing.h>
#include <stdio.h>
#include <string.h>

// 把 SQ 填满，看 get_sqe 返回什么；submit 之后空位回来。
int main(void) {
    struct io_uring ring;
    int ret = io_uring_queue_init(4, &ring, 0);
    if (ret < 0) {
        fprintf(stderr, "io_uring_queue_init: %s\n", strerror(-ret));
        return 1;
    }

    int filled = 0;
    for (; filled < 64; ++filled) {
        struct io_uring_sqe *sqe = io_uring_get_sqe(&ring);
        if (sqe == NULL) break;
        io_uring_prep_nop(sqe);
        io_uring_sqe_set_data64(sqe, (unsigned long long)filled);
    }
    struct io_uring_sqe *first = io_uring_get_sqe(&ring);
    fprintf(stderr, "filled=%d first get_sqe: %s\n", filled,
            first == NULL ? "NULL" : "not NULL");

    int submitted = io_uring_submit(&ring);
    struct io_uring_sqe *after = io_uring_get_sqe(&ring);
    fprintf(stderr, "submitted=%d second get_sqe: %s\n", submitted,
            after == NULL ? "NULL" : "not NULL");

    // 把已经提交的完成事件收掉，免得 ring 退出时还挂着请求。
    for (int i = 0; i < submitted; ++i) {
        struct io_uring_cqe *cqe;
        if (io_uring_wait_cqe(&ring, &cqe) < 0) break;
        io_uring_cqe_seen(&ring, cqe);
    }
    io_uring_queue_exit(&ring);
    return (filled == 4 && submitted == 4 && after != NULL) ? 0 : 1;
}
