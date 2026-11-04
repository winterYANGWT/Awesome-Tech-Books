// select 回射服务器：每次调用前重建 fd_set，服务两个连接后退出。
#define _GNU_SOURCE
#include <arpa/inet.h>
#include <errno.h>
#include <netinet/in.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <unistd.h>

static int listen_on(unsigned short port, const char *port_file) {
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd < 0) { perror("socket"); exit(1); }
    int yes = 1;
    setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof yes);
    struct sockaddr_in addr;
    memset(&addr, 0, sizeof addr);
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    if (bind(fd, (struct sockaddr *)&addr, sizeof addr) != 0) { perror("bind"); exit(1); }
    if (listen(fd, 8) != 0) { perror("listen"); exit(1); }
    socklen_t len = sizeof addr;
    if (getsockname(fd, (struct sockaddr *)&addr, &len) != 0) { perror("getsockname"); exit(1); }
    FILE *f = fopen(port_file, "w");
    if (f == NULL) { perror("fopen"); exit(1); }
    fprintf(f, "%u\n", ntohs(addr.sin_port));
    fclose(f);
    return fd;
}

int main(int argc, char **argv) {
    if (argc != 2) {
        fprintf(stderr, "usage: %s PORT_FILE\n", argv[0]);
        return 2;
    }
    signal(SIGPIPE, SIG_IGN);
    int lfd = listen_on(0, argv[1]);
    int conns[2] = {-1, -1};
    int accepted = 0, active = 0;
    long bytes = 0;

// ANCHOR: loop
    while (accepted < 2 || active > 0) {
        fd_set rfds;
        FD_ZERO(&rfds);
        FD_SET(lfd, &rfds);
        int maxfd = lfd;
        for (int i = 0; i < 2; ++i) {
            if (conns[i] >= 0) {
                FD_SET(conns[i], &rfds);
                if (conns[i] > maxfd) maxfd = conns[i];
            }
        }
        int n = select(maxfd + 1, &rfds, NULL, NULL, NULL);
        if (n < 0) {
            if (errno == EINTR) continue;
            perror("select");
            return 1;
        }
        if (FD_ISSET(lfd, &rfds)) {
            int fd = accept(lfd, NULL, NULL);
            if (fd >= 0 && accepted < 2) {
                conns[accepted++] = fd;
                ++active;
            } else if (fd >= 0) {
                close(fd);
            }
        }
        for (int i = 0; i < 2; ++i) {
            if (conns[i] < 0 || !FD_ISSET(conns[i], &rfds)) continue;
            char buf[64];
            ssize_t k = recv(conns[i], buf, sizeof buf, 0);
            if (k > 0) {
                bytes += k;
                ssize_t off = 0;
                while (off < k) {
                    ssize_t w = send(conns[i], buf + off, (size_t)(k - off), MSG_NOSIGNAL);
                    if (w < 0) {
                        if (errno == EINTR) continue;
                        break;
                    }
                    off += w;
                }
            } else if (k == 0 || (errno != EINTR && errno != EAGAIN)) {
                close(conns[i]);
                conns[i] = -1;
                --active;
            }
        }
    }
// ANCHOR_END: loop
    printf("accepted=%d bytes=%ld\n", accepted, bytes);
    close(lfd);
    return 0;
}
