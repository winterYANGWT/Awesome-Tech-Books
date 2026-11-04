// 控制缓冲区只留 8 字节：内核置 MSG_CTRUNC，接收方检查后拒绝使用。
#define _POSIX_C_SOURCE 200809L
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <unistd.h>

int main(void) {
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
        int fd = open("/dev/null", O_RDONLY);
        if (fd < 0) _exit(1);

        char control[CMSG_SPACE(sizeof(int))];
        memset(control, 0, sizeof control);
        char data = 'x';
        struct iovec io = {&data, 1};
        struct msghdr msg;
        memset(&msg, 0, sizeof msg);
        msg.msg_iov = &io;
        msg.msg_iovlen = 1;
        msg.msg_control = control;
        msg.msg_controllen = sizeof control;
        struct cmsghdr *cmsg = CMSG_FIRSTHDR(&msg);
        cmsg->cmsg_level = SOL_SOCKET;
        cmsg->cmsg_type = SCM_RIGHTS;
        cmsg->cmsg_len = CMSG_LEN(sizeof(int));
        memcpy(CMSG_DATA(cmsg), &fd, sizeof fd);
        if (sendmsg(sv[0], &msg, 0) < 0) _exit(1);
        close(fd);
        _exit(0);
    }

    close(sv[0]);
    char data = 0;
    struct iovec io = {&data, 1};
    char control[8];  // 故意小于 CMSG_SPACE(sizeof(int))
    struct msghdr msg;
    memset(&msg, 0, sizeof msg);
    memset(control, 0, sizeof control);
    msg.msg_iov = &io;
    msg.msg_iovlen = 1;
    msg.msg_control = control;
    msg.msg_controllen = sizeof control;

    ssize_t n = recvmsg(sv[1], &msg, 0);
    if (n < 0) {
        perror("recvmsg");
        return 1;
    }
    printf("recvmsg 返回 %zd，数据=\"%c\"，MSG_CTRUNC=%d\n", n, data,
           (msg.msg_flags & MSG_CTRUNC) ? 1 : 0);

    if (msg.msg_flags & MSG_CTRUNC) {
        printf("控制信息截断，未使用 fd\n");
    } else {
        printf("没有截断，正常遍历 cmsg\n");
    }

    close(sv[1]);
    int status = 0;
    waitpid(pid, &status, 0);
    return 0;
}
