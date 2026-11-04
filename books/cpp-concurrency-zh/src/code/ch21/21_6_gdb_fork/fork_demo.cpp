// 父进程 fork 出子进程，子进程调用 child_work()。gdb 用 follow-fork-mode
// child 跟到子进程，在 leaf() 的断点上看子进程的调用栈。
#include <cstdio>
#include <cstdlib>
#include <pthread.h>
#include <sys/wait.h>
#include <unistd.h>

__attribute__((noinline)) void leaf() {
    volatile int marker = 42;
    (void)marker;
}

__attribute__((noinline)) void child_work() {
    leaf();
}


int main() {
    std::printf("父进程启动\n");
    std::fflush(stdout);

    const pid_t pid = fork();
    if (pid < 0) {
        std::perror("fork");
        return 1;
    }
    if (pid == 0) {
        (void)pthread_setname_np(pthread_self(), "child");
        std::printf("子进程调用 child_work\n");
        std::fflush(stdout);
        child_work();
        _exit(0);
    }

    std::printf("父进程 fork 出子进程\n");
    std::fflush(stdout);
    (void)waitpid(pid, nullptr, 0);
    std::printf("父进程 wait 到子进程退出\n");
    return 0;
}
