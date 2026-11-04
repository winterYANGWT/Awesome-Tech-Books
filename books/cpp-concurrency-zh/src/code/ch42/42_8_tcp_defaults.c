// mode: run
// output: varies
// expect: tcp_syn_retries
// expect: TCP_KEEPIDLE
// 读本机的 TCP 默认值；这些数随内核版本和发行版配置变化，不能写死。
#define _GNU_SOURCE
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/utsname.h>
#include <unistd.h>

static int read_int(const char *path, int fallback) {
    FILE *f = fopen(path, "r");
    if (!f) {
        return fallback;
    }
    int v = fallback;
    if (fscanf(f, "%d", &v) != 1) {
        v = fallback;
    }
    fclose(f);
    return v;
}

static void read_line(const char *path, char *buf, size_t size) {
    FILE *f = fopen(path, "r");
    if (!f) {
        snprintf(buf, size, "(unreadable)");
        return;
    }
    if (!fgets(buf, (int)size, f)) {
        snprintf(buf, size, "(empty)");
    }
    fclose(f);
    buf[strcspn(buf, "\n")] = '\0';
}

static void read_three(const char *path) {
    FILE *f = fopen(path, "r");
    if (!f) {
        printf("%s = (unreadable)\n", path);
        return;
    }
    long a = 0, b = 0, c = 0;
    if (fscanf(f, "%ld %ld %ld", &a, &b, &c) == 3) {
        printf("%s = %ld %ld %ld\n", path, a, b, c);
    }
    fclose(f);
}

int main(void) {
    struct utsname u;
    if (uname(&u) == 0) {
        printf("kernel = %s\n", u.release);
    }

    int syn = read_int("/proc/sys/net/ipv4/tcp_syn_retries", -1);
    int synack = read_int("/proc/sys/net/ipv4/tcp_synack_retries", -1);
    printf("tcp_syn_retries = %d, tcp_synack_retries = %d\n", syn, synack);
    if (syn >= 0 && syn < 20) {
        // 初始 SYN 之后每次重传间隔翻倍：1+2+4+...
        long total = (1L << (syn + 1)) - 1;
        printf("SYN retry time ~ %ld s (exponential backoff)\n", total);
    }

    char cc[64];
    read_line("/proc/sys/net/ipv4/tcp_congestion_control", cc, sizeof cc);
    printf("tcp_congestion_control = %s\n", cc);
    read_three("/proc/sys/net/ipv4/tcp_rmem");
    read_three("/proc/sys/net/ipv4/tcp_wmem");

    int keepalive_time = read_int("/proc/sys/net/ipv4/tcp_keepalive_time", -1);
    printf("tcp_keepalive_time = %d s\n", keepalive_time);

    int fd = socket(AF_INET, SOCK_STREAM | SOCK_CLOEXEC, 0);
    if (fd >= 0) {
        int on = 0;
        socklen_t len = sizeof on;
        if (getsockopt(fd, SOL_SOCKET, SO_KEEPALIVE, &on, &len) == 0) {
            printf("SO_KEEPALIVE default = %d\n", on);
        }
        int idle = 0;
        len = sizeof idle;
        if (getsockopt(fd, IPPROTO_TCP, TCP_KEEPIDLE, &idle, &len) == 0) {
            printf("TCP_KEEPIDLE default = %d s\n", idle);
        }
        close(fd);
    }
    return 0;
}
