// mode: run
// flags: -luring
// expect: socket blocking = 1
// expect: completion res = 5
// 一个保持阻塞的 socket，用 io_uring 读：应用不调用 read，也不等可读，只收完成事件。
#define _GNU_SOURCE
#include <fcntl.h>
#include <liburing.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <unistd.h>

int main(void) {
    int sv[2];
    if (socketpair(AF_UNIX, SOCK_STREAM, 0, sv) != 0) {
        perror("socketpair");
        return 1;
    }
    printf("socket blocking = %d\n", (fcntl(sv[0], F_GETFL) & O_NONBLOCK) == 0);

    pid_t pid = fork();
    if (pid < 0) {
        perror("fork");
        return 1;
    }
    if (pid == 0) {
        close(sv[0]);
        usleep(100000);  // 让父进程先提交读请求
        if (write(sv[1], "hello", 5) != 5) {
            _exit(1);
        }
        close(sv[1]);
        _exit(0);
    }
    close(sv[1]);

    struct io_uring ring;
    int ret = io_uring_queue_init(8, &ring, 0);
    if (ret < 0) {
        fprintf(stderr, "io_uring_queue_init: %s\n", strerror(-ret));
        return 1;
    }
    printf("queue_init = %d\n", ret);

    char buf[16] = {0};
    struct io_uring_sqe *sqe = io_uring_get_sqe(&ring);
    if (!sqe) {
        return 1;
    }
    io_uring_prep_read(sqe, sv[0], buf, sizeof buf, 0);
    io_uring_submit(&ring);

    struct io_uring_cqe *cqe = NULL;
    ret = io_uring_wait_cqe(&ring, &cqe);
    printf("wait_cqe = %d\n", ret);
    printf("completion res = %d\n", cqe->res);
    printf("data = %.*s\n", cqe->res > 0 ? cqe->res : 0, buf);
    io_uring_cqe_seen(&ring, cqe);

    io_uring_queue_exit(&ring);
    close(sv[0]);
    waitpid(pid, NULL, 0);
    return 0;
}
