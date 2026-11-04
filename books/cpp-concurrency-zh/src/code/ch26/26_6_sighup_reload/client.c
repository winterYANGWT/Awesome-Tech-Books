#define _POSIX_C_SOURCE 200809L
#include <arpa/inet.h>
#include <errno.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

int main(int argc, char **argv)
{
    if (argc != 3)
        return 2;
    int fd = socket(AF_INET, SOCK_STREAM | SOCK_CLOEXEC, 0);
    if (fd == -1)
        return 1;
    struct sockaddr_in addr = {.sin_family = AF_INET,
                               .sin_port = htons((unsigned short)atoi(argv[1]))};
    if (inet_pton(AF_INET, "127.0.0.1", &addr.sin_addr) != 1 ||
        connect(fd, (struct sockaddr *)&addr, sizeof addr) == -1)
        return 1;
    char req[512];
    int rn = snprintf(req, sizeof req, "%s\n", argv[2]);
    if (rn < 0 || (size_t)rn >= sizeof req)
        return 1;
    if (send(fd, req, (size_t)rn, MSG_NOSIGNAL) == -1)
        return 1;
    char buf[512];
    ssize_t n = recv(fd, buf, sizeof buf - 1, 0);
    if (n <= 0)
        return 1;
    buf[n] = '\0';
    fputs(buf, stdout);
    close(fd);
    return 0;
}
