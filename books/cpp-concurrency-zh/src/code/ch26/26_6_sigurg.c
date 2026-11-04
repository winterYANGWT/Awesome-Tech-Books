// mode: run
// expect: 发现方式：POLLPRI=1 SIGURG=1
// expect: sockatmark=1
// expect: 普通数据=abcdef 带外数据=!
#define _GNU_SOURCE
#define _POSIX_C_SOURCE 200809L
#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <poll.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <unistd.h>

static volatile sig_atomic_t got_urg;

static void on_urg(int signo)
{
    (void)signo;
    got_urg = 1;
}

static void send_side(int port, int go_fd)
{
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd == -1)
        _exit(2);
    struct sockaddr_in addr = {.sin_family = AF_INET,
                               .sin_port = htons((unsigned short)port)};
    inet_pton(AF_INET, "127.0.0.1", &addr.sin_addr);
    if (connect(fd, (struct sockaddr *)&addr, sizeof addr) == -1)
        _exit(3);
    char go;
    if (read(go_fd, &go, 1) != 1)
        _exit(7);
    if (send(fd, "abc", 3, 0) != 3)
        _exit(4);
    if (send(fd, "!", 1, MSG_OOB) != 1)
        _exit(5);
    if (send(fd, "def", 3, 0) != 3)
        _exit(6);
    shutdown(fd, SHUT_WR);
    close(fd);
    _exit(0);
}

int main(void)
{
    int lfd = socket(AF_INET, SOCK_STREAM | SOCK_CLOEXEC, 0);
    if (lfd == -1)
        return 1;
    int one = 1;
    setsockopt(lfd, SOL_SOCKET, SO_REUSEADDR, &one, sizeof one);
    struct sockaddr_in addr = {.sin_family = AF_INET,
                               .sin_addr.s_addr = htonl(INADDR_LOOPBACK),
                               .sin_port = 0};
    if (bind(lfd, (struct sockaddr *)&addr, sizeof addr) == -1 ||
        listen(lfd, 1) == -1)
        return 1;
    socklen_t alen = sizeof addr;
    if (getsockname(lfd, (struct sockaddr *)&addr, &alen) == -1)
        return 1;
    int port = ntohs(addr.sin_port);

    struct sigaction sa = {0};
    sa.sa_handler = on_urg;
    sigemptyset(&sa.sa_mask);
    if (sigaction(SIGURG, &sa, NULL) == -1)
        return 1;

    int go[2];
    if (pipe(go) == -1)
        return 1;
    pid_t child = fork();
    if (child == -1)
        return 1;
    if (child == 0) {
        close(go[1]);
        send_side(port, go[0]);
    }
    close(go[0]);

    int cfd = accept4(lfd, NULL, NULL, SOCK_CLOEXEC);
    if (cfd == -1)
        return 1;

    sigset_t urg, empty;
    sigemptyset(&urg);
    sigaddset(&urg, SIGURG);
    if (sigprocmask(SIG_BLOCK, &urg, NULL) == -1)
        return 1;
    if (fcntl(cfd, F_SETOWN, getpid()) == -1)
        return 1;
    if (fcntl(cfd, F_SETFL, fcntl(cfd, F_GETFL) | O_NONBLOCK) == -1)
        return 1;
    if (write(go[1], "x", 1) != 1)
        return 1;
    close(go[1]);

    struct pollfd pfd = {.fd = cfd, .events = POLLIN | POLLPRI};
    int pri = 0;
    for (;;) {
        int n = poll(&pfd, 1, 2000);
        if (n == -1 && errno == EINTR)
            continue;
        if (n <= 0)
            return 1;
        if (pfd.revents & POLLPRI) {
            pri = 1;
            break;
        }
    }

    /* SIGURG 被阻塞着，poll 已经看到带外标记；放开后处理函数才会运行。 */
    sigemptyset(&empty);
    if (sigsuspend(&empty) == -1 && errno != EINTR)
        return 1;

    char normal[64];
    size_t used = 0;
    char oob = '?';
    int atmark_seen = 0;

    for (;;) {
        int at = sockatmark(cfd);
        if (at == 1) {
            atmark_seen = 1;
            break;
        }
        if (at == -1)
            return 1;
        ssize_t n = recv(cfd, normal + used, sizeof normal - 1 - used, 0);
        if (n > 0) {
            used += (size_t)n;
            normal[used] = '\0';
            continue;
        }
        if (n == -1 && (errno == EAGAIN || errno == EWOULDBLOCK)) {
            if (poll(&pfd, 1, 2000) <= 0)
                return 1;
            continue;
        }
        return 1;
    }

    if (recv(cfd, &oob, 1, MSG_OOB) != 1)
        return 1;

    for (;;) {
        ssize_t n = recv(cfd, normal + used, sizeof normal - 1 - used, 0);
        if (n > 0) {
            used += (size_t)n;
            normal[used] = '\0';
            continue;
        }
        if (n == 0)
            break;
        if (errno == EAGAIN || errno == EWOULDBLOCK) {
            if (poll(&pfd, 1, 2000) <= 0)
                return 1;
            continue;
        }
        return 1;
    }
    normal[used] = '\0';

    int status = 0;
    waitpid(child, &status, 0);
    printf("发现方式：POLLPRI=%d SIGURG=%d\n", pri, (int)got_urg);
    printf("sockatmark=%d\n", atmark_seen);
    printf("普通数据=%s 带外数据=%c\n", normal, oob);
    close(cfd);
    close(lfd);
    return 0;
}
