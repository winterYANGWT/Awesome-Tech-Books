// mode: run
// expect: 默认处置：子进程被 SIGPIPE 终止
// expect: 忽略 SIGPIPE：write=-1 errno=EPIPE
// expect: MSG_NOSIGNAL：send=-1 errno=EPIPE 正常退出
#define _POSIX_C_SOURCE 200809L
#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <unistd.h>

static void default_pipe_case(void)
{
    int p[2];
    if (pipe(p) == -1)
        _exit(10);
    close(p[0]);
    char c = 'x';
    ssize_t n = write(p[1], &c, 1);
    (void)n;
    _exit(0);
}

static void ignored_pipe_case(void)
{
    int p[2];
    struct sigaction sa = {0};
    sa.sa_handler = SIG_IGN;
    sigemptyset(&sa.sa_mask);
    if (sigaction(SIGPIPE, &sa, NULL) == -1 || pipe(p) == -1)
        _exit(11);
    close(p[0]);
    char c = 'x';
    errno = 0;
    ssize_t n = write(p[1], &c, 1);
    _exit(n == -1 && errno == EPIPE ? 0 : 12);
}

static void nosignal_socket_case(void)
{
    int sv[2];
    if (socketpair(AF_UNIX, SOCK_STREAM, 0, sv) == -1)
        _exit(13);
    close(sv[1]);
    char c = 'x';
    errno = 0;
    ssize_t n = send(sv[0], &c, 1, MSG_NOSIGNAL);
    _exit(n == -1 && errno == EPIPE ? 0 : 14);
}

static int run(void (*fn)(void))
{
    pid_t pid = fork();
    if (pid == -1)
        return -1;
    if (pid == 0)
        fn();
    int status = 0;
    if (waitpid(pid, &status, 0) == -1)
        return -1;
    return status;
}

int main(void)
{
    int st = run(default_pipe_case);
    if (WIFSIGNALED(st) && WTERMSIG(st) == SIGPIPE)
        printf("默认处置：子进程被 SIGPIPE 终止\n");
    else
        printf("默认处置：没有看到 SIGPIPE\n");

    st = run(ignored_pipe_case);
    if (WIFEXITED(st) && WEXITSTATUS(st) == 0)
        printf("忽略 SIGPIPE：write=-1 errno=EPIPE\n");
    else
        printf("忽略 SIGPIPE：结果不符合预期\n");

    st = run(nosignal_socket_case);
    if (WIFEXITED(st) && WEXITSTATUS(st) == 0)
        printf("MSG_NOSIGNAL：send=-1 errno=EPIPE 正常退出\n");
    else
        printf("MSG_NOSIGNAL：结果不符合预期\n");

    return 0;
}
