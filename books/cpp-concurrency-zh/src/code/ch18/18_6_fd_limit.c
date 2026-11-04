// mode: run
#define _GNU_SOURCE
#include <stdio.h>
#include <sys/resource.h>

static long long read_ll(const char* path) {
    FILE* f = fopen(path, "r");
    if (f == NULL) return -1;
    long long value = -1;
    if (fscanf(f, "%lld", &value) != 1) value = -1;
    fclose(f);
    return value;
}

int main(void) {
    struct rlimit rl;
    if (getrlimit(RLIMIT_NOFILE, &rl) != 0) {
        perror("getrlimit");
        return 1;
    }

    printf("RLIMIT_NOFILE soft=%llu hard=%llu  (每进程)\n",
           (unsigned long long)rl.rlim_cur, (unsigned long long)rl.rlim_max);
    printf("fs.file-max=%lld  (系统级)\n", read_ll("/proc/sys/fs/file-max"));
    printf("somaxconn=%lld  (系统级，封顶 listen backlog)\n",
           read_ll("/proc/sys/net/core/somaxconn"));
    printf("tcp_max_syn_backlog=%lld  (系统级，半连接队列)\n",
           read_ll("/proc/sys/net/ipv4/tcp_max_syn_backlog"));
    printf("max_user_watches=%lld  (每用户，注册的 fd 总数)\n",
           read_ll("/proc/sys/fs/epoll/max_user_watches"));
    return 0;
}
