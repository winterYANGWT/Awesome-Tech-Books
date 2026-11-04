// mode: run
// flags: -luring
// output: varies
// expect: completed=64 total=4096
#define _GNU_SOURCE
#include <fcntl.h>
#include <liburing.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

// 一次提交 64 个读请求，再用 peek_batch_cqe 一次取一批。
// 完成顺序不保证和提交顺序一致，输出只用来观察这一点。
enum { kRequests = 64, kBlock = 64 };

int main(void) {
    char path[] = "/tmp/uring_batch_XXXXXX";
    int file = mkstemp(path);
    if (file < 0) { perror("mkstemp"); return 1; }
    unlink(path);
    char block[kBlock];
    for (int i = 0; i < kRequests; ++i) {
        memset(block, 'A' + i % 26, sizeof block);
        if (pwrite(file, block, sizeof block, (off_t)i * kBlock) != kBlock) {
            perror("pwrite");
            return 1;
        }
    }

    struct io_uring ring;
    int ret = io_uring_queue_init(kRequests, &ring, 0);
    if (ret < 0) {
        fprintf(stderr, "io_uring_queue_init: %s\n", strerror(-ret));
        return 1;
    }

    // ANCHOR: submit
    char buffers[kRequests][kBlock];
    for (int i = 0; i < kRequests; ++i) {
        struct io_uring_sqe *sqe = io_uring_get_sqe(&ring);
        if (sqe == NULL) {
            fprintf(stderr, "SQ full at %d\n", i);
            return 1;
        }
        io_uring_prep_read(sqe, file, buffers[i], kBlock, (off_t)i * kBlock);
        io_uring_sqe_set_data64(sqe, (unsigned long long)i);
    }
    int submitted = io_uring_submit(&ring);

    struct io_uring_cqe *cqes[kRequests];
    int seen = 0;
    long long total = 0;
    int order[8] = {0};
    while (seen < kRequests) {
        unsigned n = io_uring_peek_batch_cqe(&ring, cqes, kRequests - (unsigned)seen);
        if (n == 0) {
            struct io_uring_cqe *one;
            ret = io_uring_wait_cqe(&ring, &one);
            if (ret < 0) {
                fprintf(stderr, "wait_cqe: %s\n", strerror(-ret));
                return 1;
            }
            n = 1;
            cqes[0] = one;
        }
        for (unsigned i = 0; i < n; ++i) {
            if (seen < 8) order[seen] = (int)io_uring_cqe_get_data64(cqes[i]);
            if (cqes[i]->res > 0) total += cqes[i]->res;
            io_uring_cqe_seen(&ring, cqes[i]);
            ++seen;
        }
    }

    // ANCHOR_END: submit
    fprintf(stderr, "submitted=%d completed=%d total=%lld first:", submitted,
            seen, total);
    for (int i = 0; i < 8; ++i) fprintf(stderr, " %d", order[i]);
    fputc('\n', stderr);

    close(file);
    io_uring_queue_exit(&ring);
    return total == kRequests * kBlock ? 0 : 1;
}
