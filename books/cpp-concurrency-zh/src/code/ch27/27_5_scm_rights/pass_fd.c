// 通过 AF_UNIX socket 的 SCM_RIGHTS 传 fd：接收方得到新编号，但共享文件偏移和状态标志。
#define _POSIX_C_SOURCE 200809L
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

// 把 fd 作为控制信息、把 data 作为普通数据放进同一条消息。
static int send_fd(int sock, int fd, char data) {
    struct msghdr msg;
    struct iovec io;
    char control[CMSG_SPACE(sizeof(int))];

    memset(&msg, 0, sizeof msg);
    memset(control, 0, sizeof control);
    io.iov_base = &data;
    io.iov_len = 1;
    msg.msg_iov = &io;
    msg.msg_iovlen = 1;
    msg.msg_control = control;
    msg.msg_controllen = sizeof control;

    struct cmsghdr *cmsg = CMSG_FIRSTHDR(&msg);
    cmsg->cmsg_level = SOL_SOCKET;
    cmsg->cmsg_type = SCM_RIGHTS;
    cmsg->cmsg_len = CMSG_LEN(sizeof(int));
    memcpy(CMSG_DATA(cmsg), &fd, sizeof fd);
    return (int)sendmsg(sock, &msg, 0);
}

static int recv_fd(int sock, char *data) {
    struct msghdr msg;
    struct iovec io;
    char control[CMSG_SPACE(sizeof(int))];

    memset(&msg, 0, sizeof msg);
    memset(control, 0, sizeof control);
    io.iov_base = data;
    io.iov_len = 1;
    msg.msg_iov = &io;
    msg.msg_iovlen = 1;
    msg.msg_control = control;
    msg.msg_controllen = sizeof control;

    // MSG_CMSG_CLOEXEC：收到的 fd 直接带上 FD_CLOEXEC。
    if (recvmsg(sock, &msg, MSG_CMSG_CLOEXEC) < 0) return -1;
    if (msg.msg_flags & MSG_CTRUNC) {
        fprintf(stderr, "控制信息被截断\n");
        return -1;
    }
    for (struct cmsghdr *cmsg = CMSG_FIRSTHDR(&msg); cmsg != NULL;
         cmsg = CMSG_NXTHDR(&msg, cmsg)) {
        if (cmsg->cmsg_level == SOL_SOCKET && cmsg->cmsg_type == SCM_RIGHTS) {
            int fd = -1;
            memcpy(&fd, CMSG_DATA(cmsg), sizeof fd);
            return fd;
        }
    }
    return -1;
}

int main(int argc, char **argv) {
    if (argc != 2) {
        fprintf(stderr, "usage: %s file\n", argv[0]);
        return 2;
    }
    setvbuf(stdout, NULL, _IONBF, 0);  // 两个进程交错打印，逐行刷新

    int sv[2];
    if (socketpair(AF_UNIX, SOCK_STREAM, 0, sv) < 0) {
        perror("socketpair");
        return 1;
    }

    pid_t pid = fork();
    if (pid < 0) {
        perror("fork");
        return 1;
    }

    if (pid == 0) {
        close(sv[1]);
        int fd = open(argv[1], O_RDWR | O_CREAT | O_TRUNC, 0600);
        if (fd < 0) {
            perror("open");
            _exit(1);
        }
        if (write(fd, "0123456789abcdef", 16) != 16) _exit(1);
        lseek(fd, 0, SEEK_SET);
        char head[5];
        if (read(fd, head, sizeof head) != 5) _exit(1);
        // O_NONBLOCK 是“打开文件描述”的状态，会跟着 fd 一起过去。
        fcntl(fd, F_SETFL, fcntl(fd, F_GETFL) | O_NONBLOCK);
        printf("child: 已读 5 字节 \"%.*s\"，偏移=%ld，发送 fd=%d\n", 5, head,
               (long)lseek(fd, 0, SEEK_CUR), fd);

        if (send_fd(sv[0], fd, (char)fd) < 0) {
            perror("sendmsg");
            _exit(1);
        }
        char ack;
        if (read(sv[0], &ack, 1) != 1) _exit(1);
        printf("child: 父进程读完后，我这边偏移=%ld\n", (long)lseek(fd, 0, SEEK_CUR));
        close(fd);
        _exit(0);
    }

    close(sv[0]);
    char data = 0;
    int fd = recv_fd(sv[1], &data);
    if (fd < 0) {
        perror("recvmsg");
        return 1;
    }
    printf("parent: 收到 fd=%d（子进程的 fd 是 %d），FD_CLOEXEC=%d\n", fd, (int)data,
           (fcntl(fd, F_GETFD) & FD_CLOEXEC) ? 1 : 0);
    printf("parent: O_NONBLOCK=%d（子进程设的状态也带过来了）\n",
           (fcntl(fd, F_GETFL) & O_NONBLOCK) ? 1 : 0);

    char tail[5];
    if (read(fd, tail, sizeof tail) != 5) {
        perror("read");
        return 1;
    }
    printf("parent: 接着读到 \"%.*s\"，偏移=%ld\n", 5, tail,
           (long)lseek(fd, 0, SEEK_CUR));

    if (write(sv[1], "A", 1) != 1) return 1;
    close(fd);
    int status = 0;
    waitpid(pid, &status, 0);
    close(sv[1]);
    return 0;
}
