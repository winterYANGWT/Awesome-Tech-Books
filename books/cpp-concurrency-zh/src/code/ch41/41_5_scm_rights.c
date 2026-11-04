// mode: run
// socketpair + SCM_RIGHTS 传 fd；接收端用 MSG_CMSG_CLOEXEC。传的是 memfd。
#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <sys/mman.h>
#include <unistd.h>

int main(void)
{
    int sv[2];
    int memfd;
    pid_t pid;
    const char text[] = "hello over SCM_RIGHTS";

    memfd = memfd_create("scm-demo", MFD_CLOEXEC);
    if (memfd == -1) {
        perror("memfd_create");
        return 1;
    }
    if (write(memfd, text, sizeof text - 1) != (ssize_t)(sizeof text - 1)) {
        perror("write");
        return 1;
    }
    if (lseek(memfd, 0, SEEK_SET) == -1) {
        perror("lseek");
        return 1;
    }

    if (socketpair(AF_UNIX, SOCK_DGRAM, 0, sv) == -1) {
        perror("socketpair");
        return 1;
    }

    pid = fork();
    if (pid == 0) {
        char buf[64] = {0};
        char control[CMSG_SPACE(sizeof(int))];
        struct iovec iov = {.iov_base = buf, .iov_len = 1};
        struct msghdr msg;
        struct cmsghdr *cmsg;
        int received = -1;
        ssize_t n;

        close(sv[0]);
        memset(&msg, 0, sizeof msg);
        msg.msg_iov = &iov;
        msg.msg_iovlen = 1;
        msg.msg_control = control;
        msg.msg_controllen = sizeof control;
        n = recvmsg(sv[1], &msg, MSG_CMSG_CLOEXEC);
        if (n < 0) {
            perror("recvmsg");
            _exit(1);
        }
        for (cmsg = CMSG_FIRSTHDR(&msg); cmsg != NULL;
             cmsg = CMSG_NXTHDR(&msg, cmsg)) {
            if (cmsg->cmsg_level == SOL_SOCKET &&
                cmsg->cmsg_type == SCM_RIGHTS) {
                memcpy(&received, CMSG_DATA(cmsg), sizeof received);
            }
        }
        if (received == -1) {
            printf("child: no fd received\n");
            _exit(1);
        }
        n = read(received, buf, sizeof buf - 1);
        if (n < 0) {
            perror("read");
            _exit(1);
        }
        buf[n] = '\0';
        printf("child: received fd, read: %s\n", buf);
        close(received);
        _exit(0);
    }
    if (pid == -1) {
        perror("fork");
        return 1;
    }

    close(sv[1]);
    {
        char control[CMSG_SPACE(sizeof(int))];
        struct msghdr msg;
        struct cmsghdr *cmsg;
        struct iovec iov = {.iov_base = (void *)"x", .iov_len = 1};

        memset(&msg, 0, sizeof msg);
        msg.msg_iov = &iov;
        msg.msg_iovlen = 1;
        msg.msg_control = control;
        msg.msg_controllen = sizeof control;
        cmsg = CMSG_FIRSTHDR(&msg);
        cmsg->cmsg_level = SOL_SOCKET;
        cmsg->cmsg_type = SCM_RIGHTS;
        cmsg->cmsg_len = CMSG_LEN(sizeof(int));
        memcpy(CMSG_DATA(cmsg), &memfd, sizeof memfd);
        if (sendmsg(sv[0], &msg, 0) < 0) {
            perror("sendmsg");
            return 1;
        }
    }
    printf("parent: sent 1 fd\n");
    close(memfd);
    if (waitpid(pid, NULL, 0) != pid) {
        perror("waitpid");
        return 1;
    }
    printf("parent: child exited\n");
    return 0;
}
