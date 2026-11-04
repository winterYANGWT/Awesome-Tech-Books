// 两次 fork + setsid：第一次 fork 的子进程是会话首领，再 fork 的孙进程不是。
#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>

int main(void) {
    pid_t first = fork();
    if (first < 0) {
        perror("fork");
        return 1;
    }
    if (first > 0) {
        // 守护进程化的第一步：父进程退出，让子进程被 init 收养。
        _exit(0);
    }

    if (setsid() < 0) {
        perror("setsid");
        _exit(1);
    }

    printf("first child:  getsid(0)==getpid()=%d getpgrp()==getpid()=%d\n",
           getsid(0) == getpid(), getpgrp() == getpid());
    fflush(stdout);

    pid_t second = fork();
    if (second < 0) {
        perror("fork");
        _exit(1);
    }
    if (second == 0) {
        // 第二次 fork 的孙进程不再是会话首领，也不会重新获得控制终端。
        umask(0);
        if (chdir("/") < 0) {
            perror("chdir");
            _exit(1);
        }
        printf("grandchild:   getsid(0)==getpid()=%d getpgrp()==getpid()=%d\n",
               getsid(0) == getpid(), getpgrp() == getpid());
        fflush(stdout);
        _exit(0);
    }

    int status = 0;
    if (waitpid(second, &status, 0) < 0) {
        perror("waitpid");
        _exit(1);
    }
    printf("first child:  grandchild exit=%d\n", WEXITSTATUS(status));
    return 0;
}
