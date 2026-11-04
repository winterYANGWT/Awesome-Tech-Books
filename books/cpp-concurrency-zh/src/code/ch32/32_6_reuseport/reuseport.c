// mode: run
// 4 个 worker 各自 socket + SO_REUSEPORT + bind + listen：内核按四元组分流。
#define _GNU_SOURCE
#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <poll.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <unistd.h>

#define WORKERS 4
#define CLIENTS 100

static int make_listener(unsigned short port, int reuse) {
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd < 0) { perror("socket"); exit(1); }
    int yes = 1;
    setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof yes);
    if (reuse) {
        if (setsockopt(fd, SOL_SOCKET, SO_REUSEPORT, &yes, sizeof yes) != 0) {
            perror("setsockopt(SO_REUSEPORT)");
            exit(1);
        }
    }
    struct sockaddr_in addr;
    memset(&addr, 0, sizeof addr);
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    if (bind(fd, (struct sockaddr *)&addr, sizeof addr) != 0) { perror("bind"); exit(1); }
    if (listen(fd, 256) != 0) { perror("listen"); exit(1); }
    return fd;
}

static void set_nonblock(int fd) {
    int flags = fcntl(fd, F_GETFL, 0);
    fcntl(fd, F_SETFL, flags | O_NONBLOCK);
}

static void worker(unsigned short port, int ready_fd, int stop_fd,
                   int result_fd) {
    int s = make_listener(port, 1);
    set_nonblock(s);
    char c = 'R';
    if (write(ready_fd, &c, 1) != 1) _exit(1);

    int count = 0;
    for (;;) {
        struct pollfd p[2];
        p[0].fd = s;
        p[0].events = POLLIN;
        p[0].revents = 0;
        p[1].fd = stop_fd;
        p[1].events = POLLIN;
        p[1].revents = 0;
        int n = poll(p, 2, 1000);
        if (n < 0) {
            if (errno == EINTR) continue;
            break;
        }
        for (;;) {
            int cfd = accept(s, NULL, NULL);
            if (cfd < 0) break;
            ++count;
            close(cfd);
        }
        if (p[1].revents & POLLIN) break;
    }
    if (write(result_fd, &count, sizeof count) != (ssize_t)sizeof count) _exit(1);
    close(s);
    _exit(0);
}

int main(void) {
    int probe = make_listener(0, 1);
    struct sockaddr_in addr;
    socklen_t len = sizeof addr;
    if (getsockname(probe, (struct sockaddr *)&addr, &len) != 0) { perror("getsockname"); return 1; }
    unsigned short port = ntohs(addr.sin_port);

    int ready[2], result[2], stop[WORKERS][2];
    if (pipe(ready) != 0 || pipe(result) != 0) { perror("pipe"); return 1; }
    for (int i = 0; i < WORKERS; ++i)
        if (pipe(stop[i]) != 0) { perror("pipe"); return 1; }

    pid_t pids[WORKERS];
    for (int i = 0; i < WORKERS; ++i) {
        pids[i] = fork();
        if (pids[i] < 0) { perror("fork"); return 1; }
        if (pids[i] == 0) {
            close(ready[0]);
            close(result[0]);
            for (int j = 0; j < WORKERS; ++j) close(stop[j][1]);
            close(probe);
            worker(port, ready[1], stop[i][0], result[1]);
        }
    }

    close(ready[1]);
    close(result[1]);
    for (int i = 0; i < WORKERS; ++i) close(stop[i][0]);

    char c;
    for (int i = 0; i < WORKERS; ++i) {
        if (read(ready[0], &c, 1) != 1) { perror("ready"); return 1; }
    }
    close(probe);  // 父进程保持的监听 socket 也关掉，只留 4 个 worker 的

    for (int i = 0; i < CLIENTS; ++i) {
        int fd = socket(AF_INET, SOCK_STREAM, 0);
        if (connect(fd, (struct sockaddr *)&addr, sizeof addr) != 0) {
            perror("connect");
            return 1;
        }
        close(fd);
    }
    usleep(200 * 1000);

    for (int i = 0; i < WORKERS; ++i) {
        if (write(stop[i][1], "s", 1) != 1) { perror("stop"); return 1; }
        close(stop[i][1]);
    }

    int total = 0;
    for (int i = 0; i < WORKERS; ++i) {
        int n = 0;
        if (read(result[0], &n, sizeof n) != (ssize_t)sizeof n) { perror("result"); return 1; }
        printf("worker %d: %d\n", i, n);
        total += n;
    }
    printf("total=%d\n", total);

    for (int i = 0; i < WORKERS; ++i) waitpid(pids[i], NULL, 0);
    return 0;
}
