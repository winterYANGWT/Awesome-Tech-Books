// POSIX 消息队列 + mq_notify 的 SIGEV_SIGNAL：通知一次，取一条，再注册一次。
#define _POSIX_C_SOURCE 200809L
#include <fcntl.h>
#include <mqueue.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

// 一轮：注册通知、fork 一个发送者、等信号、再取消息。
static void round_trip(mqd_t mqd, const char *text, int *signals) {
    struct sigevent sev;
    memset(&sev, 0, sizeof sev);
    sev.sigev_notify = SIGEV_SIGNAL;
    sev.sigev_signo = SIGUSR1;
    if (mq_notify(mqd, &sev) < 0) {
        perror("mq_notify");
        exit(1);
    }

    pid_t pid = fork();
    if (pid < 0) {
        perror("fork");
        exit(1);
    }
    if (pid == 0) {
        if (mq_send(mqd, text, strlen(text) + 1, 1) < 0) {
            perror("mq_send");
            _exit(1);
        }
        _exit(0);
    }

    sigset_t set;
    sigemptyset(&set);
    sigaddset(&set, SIGUSR1);
    siginfo_t info;
    if (sigwaitinfo(&set, &info) < 0) {
        perror("sigwaitinfo");
        exit(1);
    }
    ++*signals;
    printf("收到通知 %d: signal=%d si_code=%s\n", *signals, info.si_signo,
           info.si_code == SI_MESGQ ? "SI_MESGQ" : "其他");

    char buf[64];
    unsigned prio = 0;
    ssize_t n = mq_receive(mqd, buf, sizeof buf, &prio);
    if (n < 0) {
        perror("mq_receive");
        exit(1);
    }
    printf("mq_receive 返回 %zd，内容=\"%s\"，优先级=%u\n", n, buf, prio);

    int status = 0;
    waitpid(pid, &status, 0);
}

int main(void) {
    char name[64];
    snprintf(name, sizeof name, "/kvd-ch27-mq-%d", (int)getpid());

    struct mq_attr attr;
    memset(&attr, 0, sizeof attr);
    attr.mq_maxmsg = 4;
    attr.mq_msgsize = 64;
    mqd_t mqd = mq_open(name, O_CREAT | O_EXCL | O_RDWR, 0600, &attr);
    if (mqd == (mqd_t)-1) {
        perror("mq_open");
        return 1;
    }
    mq_unlink(name);  // 名字删掉，fd 仍然有效

    // 信号要阻塞，交给 sigwaitinfo 同步收取，避免处理函数打断主流程。
    sigset_t set;
    sigemptyset(&set);
    sigaddset(&set, SIGUSR1);
    if (sigprocmask(SIG_BLOCK, &set, NULL) < 0) {
        perror("sigprocmask");
        return 1;
    }

    int signals = 0;
    round_trip(mqd, "hello", &signals);
    round_trip(mqd, "world", &signals);  // 通知是一次性的，必须重新注册

    printf("总共收到 %d 次通知\n", signals);
    mq_close(mqd);
    return 0;
}
