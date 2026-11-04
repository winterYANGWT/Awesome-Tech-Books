// mode: run
// flags: -luring
// expect: recv: submitted=1 res=4
#define _GNU_SOURCE
#include <arpa/inet.h>
#include <liburing.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <unistd.h>

// 一个连接上的四步——accept、recv、send、close——全部提交到同一个 ring。
// 子进程只当客户端，好让父进程的四步可以按顺序写出来。
static void client(unsigned short port) {
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd < 0) _exit(1);
    struct sockaddr_in addr = {0};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    if (connect(fd, (struct sockaddr *)&addr, sizeof addr) < 0) _exit(2);
    if (send(fd, "ping", 4, 0) != 4) _exit(3);
    char buf[8];
    ssize_t n = recv(fd, buf, sizeof buf, 0);
    if (n != 4 || memcmp(buf, "ping", 4) != 0) _exit(4);
    close(fd);
    _exit(0);
}

static int step(struct io_uring *ring, ssize_t *res) {
    struct io_uring_cqe *cqe;
    int ret = io_uring_wait_cqe(ring, &cqe);
    if (ret < 0) return ret;
    *res = cqe->res;
    io_uring_cqe_seen(ring, cqe);
    return 0;
}

int main(void) {
    int listener = socket(AF_INET, SOCK_STREAM | SOCK_CLOEXEC, 0);
    if (listener < 0) { perror("socket"); return 1; }
    int yes = 1;
    setsockopt(listener, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof yes);
    struct sockaddr_in addr = {0};
    addr.sin_family = AF_INET;
    addr.sin_port = 0;  // 让内核选端口
    addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    if (bind(listener, (struct sockaddr *)&addr, sizeof addr) < 0) { perror("bind"); return 1; }
    if (listen(listener, 8) < 0) { perror("listen"); return 1; }
    socklen_t len = sizeof addr;
    if (getsockname(listener, (struct sockaddr *)&addr, &len) < 0) { perror("getsockname"); return 1; }

    pid_t child = fork();
    if (child < 0) { perror("fork"); return 1; }
    if (child == 0) client(ntohs(addr.sin_port));

    // fork 之后再建 ring：子进程不继承 ring 的映射。
    struct io_uring ring;
    int ret = io_uring_queue_init(8, &ring, 0);
    if (ret < 0) {
        fprintf(stderr, "io_uring_queue_init: %s\n", strerror(-ret));
        return 1;
    }

    struct io_uring_sqe *sqe;
    ssize_t res;

    // ANCHOR: steps
    sqe = io_uring_get_sqe(&ring);
    io_uring_prep_accept(sqe, listener, NULL, NULL, 0);
    io_uring_sqe_set_data64(sqe, 1);
    io_uring_submit(&ring);
    step(&ring, &res);
    if (res < 0) { fprintf(stderr, "accept: %s\n", strerror((int)-res)); return 1; }
    // res 是接受的 fd 号，随进程打开的文件数变化，所以这里只说成功。
    fputs("accept: res=ok\n", stderr);
    int conn = (int)res;

    char buf[16];
    sqe = io_uring_get_sqe(&ring);
    io_uring_prep_recv(sqe, conn, buf, sizeof buf, 0);
    io_uring_sqe_set_data64(sqe, 2);
    int submitted = io_uring_submit(&ring);
    step(&ring, &res);
    fprintf(stderr, "recv: submitted=%d res=%zd\n", submitted, res);
    ssize_t n = res;

    sqe = io_uring_get_sqe(&ring);
    io_uring_prep_send(sqe, conn, buf, (unsigned)n, 0);
    io_uring_sqe_set_data64(sqe, 3);
    submitted = io_uring_submit(&ring);
    step(&ring, &res);
    fprintf(stderr, "send: submitted=%d res=%zd\n", submitted, res);

    sqe = io_uring_get_sqe(&ring);
    io_uring_prep_close(sqe, conn);
    io_uring_sqe_set_data64(sqe, 4);
    submitted = io_uring_submit(&ring);
    step(&ring, &res);
    fprintf(stderr, "close: submitted=%d res=%zd\n", submitted, res);
    // ANCHOR_END: steps

    close(listener);
    io_uring_queue_exit(&ring);

    int status = 0;
    waitpid(child, &status, 0);
    return (WIFEXITED(status) && WEXITSTATUS(status) == 0) ? 0 : 1;
}
