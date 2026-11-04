// mode: run
// 模拟 systemd socket 激活：监听 fd 由父进程准备好，dup2 到 3，再 exec 服务。
#define _POSIX_C_SOURCE 200809L

#include <arpa/inet.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <unistd.h>

static int make_listener(unsigned short *port)
{
    struct sockaddr_in addr;
    socklen_t len = sizeof addr;
    int on = 1;
    int fd = socket(AF_INET, SOCK_STREAM, 0);

    if (fd == -1) {
        return -1;
    }
    setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof on);
    memset(&addr, 0, sizeof addr);
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    if (bind(fd, (struct sockaddr *)&addr, sizeof addr) == -1 ||
        listen(fd, 8) == -1 ||
        getsockname(fd, (struct sockaddr *)&addr, &len) == -1) {
        close(fd);
        return -1;
    }
    *port = ntohs(addr.sin_port);
    return fd;
}

// ANCHOR: service
static int run_service(void)
{
    const char *fds = getenv("LISTEN_FDS");
    const char *pid_s = getenv("LISTEN_PID");
    char buf[64];
    ssize_t n;
    int conn;

    printf("service: LISTEN_FDS=%s\n", fds != NULL ? fds : "(unset)");
    if (fds == NULL || atoi(fds) != 1 || pid_s == NULL ||
        (pid_t)atol(pid_s) != getpid()) {
        printf("service: LISTEN_PID mismatch\n");
        return 1;
    }

    conn = accept(3, NULL, NULL);      /* 监听 fd 就是 3，不再 bind/listen */
    if (conn == -1) {
        perror("accept");
        return 1;
    }
    n = read(conn, buf, sizeof buf);
    if (n > 0 && write(conn, buf, (size_t)n) == n) {
        printf("service: echoed %zd bytes\n", n);
    }
    close(conn);
    return 0;
}

// ANCHOR_END: service

int main(int argc, char **argv)
{
    unsigned short port = 0;
    int listener;
    pid_t pid;
    int status = 0;

    if (argc > 1 && strcmp(argv[1], "service") == 0) {
        return run_service();
    }
    if (argc < 3) {
        fprintf(stderr, "usage: %s parent PORTFILE | service\n", argv[0]);
        return 2;
    }

    listener = make_listener(&port);
    if (listener == -1) {
        perror("listener");
        return 1;
    }
    if (dup2(listener, 3) == -1) {
        perror("dup2");
        return 1;
    }
    if (listener != 3) {
        close(listener);
    }
    /* systemd 传进来的 fd 不带 FD_CLOEXEC，否则 exec 后就没了 */
    if (fcntl(3, F_SETFD, 0) == -1) {
        perror("fcntl");
        return 1;
    }

    {
        FILE *f = fopen(argv[2], "w");
        if (f == NULL) {
            perror("fopen");
            return 1;
        }
        fprintf(f, "%u\n", port);
        fclose(f);
    }
    printf("listen_fds=1\n");
    fflush(stdout);

    pid = fork();
    if (pid == 0) {
        char num[32];

        snprintf(num, sizeof num, "%ld", (long)getpid());
        setenv("LISTEN_PID", num, 1);
        setenv("LISTEN_FDS", "1", 1);
        /* fd 3 没有 FD_CLOEXEC，exec 后服务能直接 accept */
        execl(argv[0], argv[0], "service", (char *)NULL);
        _exit(127);
    }
    if (pid == -1) {
        perror("fork");
        return 1;
    }

    close(3);
    if (waitpid(pid, &status, 0) != pid) {
        perror("waitpid");
        return 1;
    }
    printf("server: child exit status=%d\n",
           WIFEXITED(status) ? WEXITSTATUS(status) : -1);
    return 0;
}
