// 按 type 3、1、2 发送，接收端按 1、2、3 取：出队顺序和发送顺序不同。
// msgrcv 成功时返回复制到 mtext 的字节数，不是 0。
#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/wait.h>
#include <unistd.h>

struct Msg {
    long mtype;
    char mtext[32];
};

static void send_msg(int qid, long type, const char *text) {
    struct Msg msg;
    msg.mtype = type;
    snprintf(msg.mtext, sizeof msg.mtext, "%s", text);
    if (msgsnd(qid, &msg, strlen(msg.mtext) + 1, 0) < 0) {
        perror("msgsnd");
        exit(1);
    }
}

static void recv_msg(int qid, long type) {
    struct Msg msg;
    ssize_t n = msgrcv(qid, &msg, sizeof msg.mtext, type, 0);
    if (n < 0) {
        perror("msgrcv");
        exit(1);
    }
    printf("msgrcv(type=%ld) 返回 %zd，mtype=%ld，内容=\"%s\"\n",
           type, n, msg.mtype, msg.mtext);
}

int main(void) {
    int qid = msgget(IPC_PRIVATE, IPC_CREAT | 0600);
    if (qid < 0) {
        perror("msgget");
        return 1;
    }

    pid_t pid = fork();
    if (pid < 0) {
        perror("fork");
        return 1;
    }
    if (pid == 0) {
        send_msg(qid, 3, "three");
        send_msg(qid, 1, "one");
        send_msg(qid, 2, "two");
        printf("child: 发送顺序 3, 1, 2\n");
        fflush(stdout);
        _exit(0);
    }

    int status = 0;
    waitpid(pid, &status, 0);

    recv_msg(qid, 1);
    recv_msg(qid, 2);
    recv_msg(qid, 3);

    // msgtype 小于 0：取类型值不超过 |msgtype| 的第一条（类型最小者）。
    send_msg(qid, 30, "thirty");
    send_msg(qid, 10, "ten");
    send_msg(qid, 20, "twenty");
    recv_msg(qid, -25);  // 10 和 20 都 <= 25，先取最小的 10
    recv_msg(qid, -25);  // 再取 20；剩下的 30 不满足 <= 25

    msgctl(qid, IPC_RMID, NULL);
    return 0;
}
