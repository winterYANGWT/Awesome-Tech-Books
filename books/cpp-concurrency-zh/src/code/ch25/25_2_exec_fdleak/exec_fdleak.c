// 监听 fd 没有 CLOEXEC 时会被 exec 带进子进程，父进程关掉自己的副本也收不回端口。
#define _GNU_SOURCE

#include <arpa/inet.h>
#include <errno.h>
#include <netinet/in.h>
#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

static void sleep_ms(long ms) {
    struct timespec ts;
    ts.tv_sec = ms / 1000;
    ts.tv_nsec = (ms % 1000) * 1000000L;
    nanosleep(&ts, NULL);
}

static int make_listener(int cloexec, unsigned short *port) {
    int type = SOCK_STREAM | (cloexec ? SOCK_CLOEXEC : 0);
    int fd = socket(AF_INET, type, 0);
    if (fd < 0) {
        perror("socket");
        return -1;
    }

    int yes = 1;
    if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof yes) < 0) {
        perror("setsockopt");
        close(fd);
        return -1;
    }

    struct sockaddr_in addr;
    memset(&addr, 0, sizeof addr);
    addr.sin_family = AF_INET;
    addr.sin_port = 0;
    addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    if (bind(fd, (struct sockaddr *)&addr, sizeof addr) < 0) {
        perror("bind");
        close(fd);
        return -1;
    }
    if (listen(fd, 8) < 0) {
        perror("listen");
        close(fd);
        return -1;
    }

    socklen_t len = sizeof addr;
    if (getsockname(fd, (struct sockaddr *)&addr, &len) < 0) {
        perror("getsockname");
        close(fd);
        return -1;
    }
    *port = ntohs(addr.sin_port);
    return fd;
}

static int try_bind(unsigned short port) {
    int fd = socket(AF_INET, SOCK_STREAM | SOCK_CLOEXEC, 0);
    if (fd < 0) {
        return -1;
    }

    int yes = 1;
    setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof yes);

    struct sockaddr_in addr;
    memset(&addr, 0, sizeof addr);
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    if (bind(fd, (struct sockaddr *)&addr, sizeof addr) < 0) {
        int saved = errno;
        close(fd);
        errno = saved;
        return -1;
    }
    close(fd);
    return 0;
}

static void one_case(const char *label, int cloexec) {
    unsigned short port = 0;
    int lfd = make_listener(cloexec, &port);
    if (lfd < 0) {
        return;
    }

    pid_t pid = fork();
    if (pid < 0) {
        perror("fork");
        close(lfd);
        return;
    }
    if (pid == 0) {
        // sleep 不关心 fd，但 exec 会把没有 CLOEXEC 的监听 fd 一起带过去。
        execl("/bin/sleep", "sleep", "1", (char *)NULL);
        _exit(127);
    }

    close(lfd);
    sleep_ms(100);  // 等子进程 exec，监听 fd 该在的已经在，该关的已经关

    if (try_bind(port) < 0) {
        printf("%s: 重新 bind -> %s\n", label, strerror(errno));
    } else {
        printf("%s: 重新 bind -> ok\n", label);
    }

    kill(pid, SIGTERM);
    waitpid(pid, NULL, 0);
}

int main(void) {
    one_case("没有 CLOEXEC", 0);
    one_case("SOCK_CLOEXEC", 1);
    return 0;
}
