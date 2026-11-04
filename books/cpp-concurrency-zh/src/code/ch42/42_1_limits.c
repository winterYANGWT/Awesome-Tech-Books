// mode: run
// output: varies
// expect: poll\(nfds=70000, last fd readable\) = 1
// 打印编译期常量、每进程限制和系统级限制；最后一项可读，证明 poll 扫到了第 70000 项。
#define _GNU_SOURCE
#include <errno.h>
#include <poll.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/resource.h>
#include <sys/select.h>
#include <unistd.h>

static void print_file(const char *path, const char *label) {
    FILE *f = fopen(path, "r");
    if (!f) {
        printf("%s = (cannot read: %s)\n", label, strerror(errno));
        return;
    }
    char buf[64] = {0};
    if (fgets(buf, sizeof buf, f)) {
        printf("%s = %s", label, buf);
    }
    fclose(f);
}

int main(void) {
    printf("FD_SETSIZE = %d, sizeof(fd_set) = %zu\n", FD_SETSIZE, sizeof(fd_set));
    printf("sizeof(nfds_t) = %zu\n", sizeof(nfds_t));

    struct rlimit rl;
    if (getrlimit(RLIMIT_NOFILE, &rl) != 0) {
        perror("getrlimit");
        return 1;
    }
    printf("RLIMIT_NOFILE soft = %llu, hard = %llu\n",
           (unsigned long long)rl.rlim_cur, (unsigned long long)rl.rlim_max);

    print_file("/proc/sys/fs/file-max", "fs.file-max");
    print_file("/proc/sys/fs/nr_open", "fs.nr_open");

    // 70000 个项，前 69999 个 fd=-1 被内核忽略，最后一项可读：
    // 只有 nfds 真的传到了 70000，poll 才会返回 1。
    nfds_t n = 70000;
    struct pollfd *fds = malloc(sizeof(*fds) * n);
    if (!fds) {
        return 1;
    }
    int p[2];
    if (pipe(p) != 0) {
        perror("pipe");
        return 1;
    }
    char byte = 'x';
    if (write(p[1], &byte, 1) != 1) {
        perror("write");
        return 1;
    }
    for (nfds_t i = 0; i < n; i++) {
        fds[i].fd = -1;
        fds[i].events = POLLIN;
        fds[i].revents = 0;
    }
    fds[n - 1].fd = p[0];
    fds[n - 1].events = POLLIN;
    int ret = poll(fds, n, 0);
    printf("poll(nfds=70000, last fd readable) = %d\n", ret);
    free(fds);
    close(p[0]);
    close(p[1]);
    return 0;
}
