// expect: stream: read 6 bytes: "AAAABB"
// expect: seqpacket: read 4 bytes: "AAAA"
// SOCK_STREAM 不保留消息边界，一次 read 可能拿到两条消息的一半；
// SOCK_SEQPACKET 保留边界，每次 read 正好一条消息。
#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <unistd.h>

// 父进程先把 3 条 4 字节消息写进 socket，再往 sync 管道写一个字节；
// 子进程等到这个字节才开始读。这样 12 字节一定已经在缓冲区里，输出确定。
static void run(int type, const char *name) {
    int sv[2];
    int sync[2];
    if (socketpair(AF_UNIX, type, 0, sv) < 0 || pipe(sync) < 0) {
        perror("socketpair/pipe");
        exit(1);
    }

    pid_t pid = fork();
    if (pid < 0) {
        perror("fork");
        exit(1);
    }

    if (pid == 0) {
        close(sv[1]);
        close(sync[1]);
        char go;
        if (read(sync[0], &go, 1) != 1) _exit(1);  // 等父进程把消息都写完
        close(sync[0]);

        char buf[6];
        for (int i = 0; i < 3; ++i) {
            ssize_t n = read(sv[0], buf, sizeof buf);
            if (n < 0) {
                perror("read");
                _exit(1);
            }
            if (n == 0) {
                printf("%s: read 0 bytes (EOF)\n", name);
            } else {
                printf("%s: read %zd bytes: \"%.*s\"\n", name, n, (int)n, buf);
            }
            fflush(stdout);
        }
        close(sv[0]);
        _exit(0);
    }

    close(sv[0]);
    close(sync[0]);
    if (write(sv[1], "AAAA", 4) != 4 || write(sv[1], "BBBB", 4) != 4 ||
        write(sv[1], "CCCC", 4) != 4 || write(sync[1], "x", 1) != 1) {
        perror("write");
        exit(1);
    }
    close(sync[1]);
    close(sv[1]);

    int status = 0;
    waitpid(pid, &status, 0);
}

int main(void) {
    run(SOCK_STREAM, "stream");
    run(SOCK_SEQPACKET, "seqpacket");
    return 0;
}
