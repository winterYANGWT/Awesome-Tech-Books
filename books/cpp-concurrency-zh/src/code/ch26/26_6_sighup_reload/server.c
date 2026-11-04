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
#include <unistd.h>

static int sigpipe_write = -1;

static void on_signal(int signo)
{
    int saved = errno;
    unsigned char b = (unsigned char)signo;
    if (sigpipe_write >= 0) {
        ssize_t n = write(sigpipe_write, &b, 1);
        (void)n;
    }
    errno = saved;
}

static int load_mode(const char *path, char *out, size_t cap)
{
    FILE *f = fopen(path, "r");
    if (!f)
        return -1;
    if (!fgets(out, (int)cap, f)) {
        fclose(f);
        return -1;
    }
    fclose(f);
    out[strcspn(out, "\r\n")] = '\0';
    return 0;
}

static int make_listener(void)
{
    int fd = socket(AF_INET, SOCK_STREAM | SOCK_CLOEXEC, 0);
    if (fd == -1)
        return -1;
    int one = 1;
    setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &one, sizeof one);
    struct sockaddr_in addr = {.sin_family = AF_INET,
                               .sin_addr.s_addr = htonl(INADDR_LOOPBACK),
                               .sin_port = 0};
    if (bind(fd, (struct sockaddr *)&addr, sizeof addr) == -1 ||
        listen(fd, 16) == -1)
        return -1;
    return fd;
}

static int write_port_file(const char *path, int fd)
{
    struct sockaddr_in addr;
    socklen_t len = sizeof addr;
    if (getsockname(fd, (struct sockaddr *)&addr, &len) == -1)
        return -1;
    FILE *f = fopen(path, "w");
    if (!f)
        return -1;
    fprintf(f, "%u\n", (unsigned)ntohs(addr.sin_port));
    fclose(f);
    return 0;
}

static void serve_one(int cfd, const char *mode)
{
    char req[512];
    size_t used = 0;
    while (used + 1 < sizeof req) {
        ssize_t n = recv(cfd, req + used, sizeof req - 1 - used, 0);
        if (n == -1 && errno == EINTR)
            continue;
        if (n <= 0)
            break;
        used += (size_t)n;
        req[used] = '\0';
        if (strchr(req, '\n'))
            break;
    }
    req[used] = '\0';
    char reply[600];
    if (strcmp(req, "GET mode\n") == 0)
        snprintf(reply, sizeof reply, "VALUE %s\n", mode);
    else
        snprintf(reply, sizeof reply, "ERROR bad request\n");
    const char *p = reply;
    size_t left = strlen(reply);
    while (left > 0) {
        ssize_t n = send(cfd, p, left, MSG_NOSIGNAL);
        if (n == -1 && errno == EINTR)
            continue;
        if (n <= 0)
            break;
        p += n;
        left -= (size_t)n;
    }
}

int main(int argc, char **argv)
{
    if (argc != 3) {
        fprintf(stderr, "usage: %s portfile config\n", argv[0]);
        return 2;
    }
    int p[2];
    if (pipe2(p, O_NONBLOCK | O_CLOEXEC) == -1)
        return 1;
    sigpipe_write = p[1];

    struct sigaction sa = {0};
    sa.sa_handler = on_signal;
    sigemptyset(&sa.sa_mask);
    if (sigaction(SIGHUP, &sa, NULL) == -1 ||
        sigaction(SIGTERM, &sa, NULL) == -1 ||
        sigaction(SIGINT, &sa, NULL) == -1)
        return 1;
    signal(SIGPIPE, SIG_IGN);

    char mode[64] = "unset";
    if (load_mode(argv[2], mode, sizeof mode) == -1)
        return 1;
    int lfd = make_listener();
    if (lfd == -1 || write_port_file(argv[1], lfd) == -1)
        return 1;

    int running = 1;
    while (running) {
        struct pollfd fds[2] = {{.fd = lfd, .events = POLLIN},
                                {.fd = p[0], .events = POLLIN}};
        int n = poll(fds, 2, 1000);
        if (n == -1) {
            if (errno == EINTR)
                continue;
            break;
        }
        if (fds[1].revents & POLLIN) {
            unsigned char buf[16];
            ssize_t got = read(p[0], buf, sizeof buf);
            for (ssize_t i = 0; i < got; ++i) {
                if (buf[i] == SIGHUP)
                    (void)load_mode(argv[2], mode, sizeof mode);
                else if (buf[i] == SIGTERM || buf[i] == SIGINT)
                    running = 0;
            }
        }
        if ((fds[0].revents & POLLIN) && running) {
            int cfd = accept4(lfd, NULL, NULL, SOCK_CLOEXEC);
            if (cfd >= 0) {
                serve_one(cfd, mode);
                close(cfd);
            }
        }
    }
    close(lfd);
    close(p[0]);
    close(p[1]);
    return 0;
}
