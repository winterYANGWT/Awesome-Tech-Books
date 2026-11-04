// 进程池：父进程只监听 listen fd，收到连接后通过 socketpair 通知一个子进程；
// 子进程自己 accept。子进程退出后父进程 SIGCHLD 回收并补一个。
#include "net.hpp"

#include <pthread.h>
#include <signal.h>
#include <sys/epoll.h>
#include <sys/signalfd.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <unistd.h>

#include <cerrno>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <system_error>

namespace {

constexpr int kMaxChildren = 8;
constexpr int kNotifyByte = 1;
constexpr int kAckByte = 1;

struct ChildInfo {
    pid_t pid = -1;
    int parent_fd = -1;  // 父进程这一端
    int child_fd = -1;   // fork 前暂存，父进程随后关掉
};

ChildInfo children[kMaxChildren];
int child_count = 4;
int listen_fd = -1;
int epfd = -1;
int sig_fd = -1;
int next_child = 0;
int notified = -1;
bool pending = false;
bool stopping = false;

std::uint16_t parse_port(const char *text) {
    char *end = nullptr;
    unsigned long v = std::strtoul(text, &end, 10);
    if (end == text || *end != '\0' || v > 65535) {
        std::fprintf(stderr, "bad port: %s\n", text);
        std::exit(2);
    }
    return static_cast<std::uint16_t>(v);
}

int live_children() {
    int n = 0;
    for (int i = 0; i < child_count; ++i) {
        if (children[i].pid > 0) ++n;
    }
    return n;
}

void add_fd(int fd, std::uint32_t events) {
    epoll_event ev{};
    ev.events = events;
    ev.data.fd = fd;
    if (epoll_ctl(epfd, EPOLL_CTL_ADD, fd, &ev) != 0)
        throw std::system_error(errno, std::generic_category(), "epoll_ctl(add)");
}

// fork 出的子进程只保留自己的 socketpair 端和 listen fd。
void child_cleanup(int keep_index, int keep_fd) {
    for (int i = 0; i < kMaxChildren; ++i) {
        if (children[i].parent_fd >= 0) close(children[i].parent_fd);
        if (children[i].child_fd >= 0 && children[i].child_fd != keep_fd)
            close(children[i].child_fd);
    }
    (void)keep_index;
    if (epfd >= 0) close(epfd);
    if (sig_fd >= 0) close(sig_fd);
}

[[noreturn]] void run_child(int index, int notify_fd) {
    const int cep = epoll_create1(EPOLL_CLOEXEC);
    if (cep < 0) _exit(1);

    epoll_event ev{};
    ev.events = EPOLLIN;
    ev.data.fd = notify_fd;
    epoll_ctl(cep, EPOLL_CTL_ADD, notify_fd, &ev);

    sigset_t mask;
    sigemptyset(&mask);
    sigaddset(&mask, SIGTERM);
    sigaddset(&mask, SIGINT);
    const int child_sig = signalfd(-1, &mask, SFD_NONBLOCK | SFD_CLOEXEC);
    if (child_sig < 0) _exit(1);
    ev.events = EPOLLIN;
    ev.data.fd = child_sig;
    epoll_ctl(cep, EPOLL_CTL_ADD, child_sig, &ev);

    std::uint64_t accepts = 0;
    epoll_event events[4];
    for (;;) {
        const int n = epoll_wait(cep, events, 4, -1);
        if (n < 0) {
            if (errno == EINTR) continue;
            _exit(1);
        }
        for (int i = 0; i < n; ++i) {
            if (events[i].data.fd == child_sig) {
                signalfd_siginfo info{};
                if (read(child_sig, &info, sizeof info) == sizeof info &&
                    (info.ssi_signo == SIGTERM || info.ssi_signo == SIGINT)) {
                    std::printf("child index=%d pid=%d accepts=%llu\n", index,
                                static_cast<int>(getpid()),
                                static_cast<unsigned long long>(accepts));
                    std::fflush(stdout);
                    _exit(0);
                }
                continue;
            }

            char notify = 0;
            if (read(notify_fd, &notify, 1) != 1) continue;
            // 一次通知只 accept 一个连接：父进程的 Round Robin 才有意义。
            // 如果这里循环 accept 到 EAGAIN，一个子进程会把积压的连接全抢走。
            int conn = -1;
            do {
                conn = accept4(listen_fd, nullptr, nullptr, SOCK_CLOEXEC);
            } while (conn < 0 && errno == EINTR);
            if (conn >= 0) {
                ++accepts;
                char byte = 0;
                if (read(conn, &byte, 1) == 1) {
                    const ssize_t sent = write(conn, &byte, 1);
                    (void)sent;
                }
                close(conn);
            }
            const char ack = kAckByte;
            const ssize_t sent = write(notify_fd, &ack, 1);
            (void)sent;
        }
    }
}

void spawn_child(int index) {
    int pair[2];
    if (socketpair(AF_UNIX, SOCK_STREAM | SOCK_CLOEXEC | SOCK_NONBLOCK, 0, pair) != 0)
        throw std::system_error(errno, std::generic_category(), "socketpair");

    children[index].parent_fd = pair[0];
    children[index].child_fd = pair[1];
    const pid_t pid = fork();
    if (pid < 0) throw std::system_error(errno, std::generic_category(), "fork");
    if (pid == 0) {
        child_cleanup(index, pair[1]);
        run_child(index, pair[1]);
    }
    close(pair[1]);
    children[index].child_fd = -1;
    children[index].pid = pid;
    add_fd(pair[0], EPOLLIN);
}

void rearm_listen() {
    epoll_event ev{};
    ev.events = EPOLLIN | EPOLLONESHOT;
    ev.data.fd = listen_fd;
    epoll_ctl(epfd, EPOLL_CTL_MOD, listen_fd, &ev);
}

void notify_next() {
    if (pending || stopping) return;
    for (int attempt = 0; attempt < child_count; ++attempt) {
        const int index = next_child;
        next_child = (next_child + 1) % child_count;
        if (children[index].pid <= 0 || children[index].parent_fd < 0) continue;
        const char byte = kNotifyByte;
        const ssize_t n = write(children[index].parent_fd, &byte, 1);
        if (n == 1) {
            pending = true;
            notified = index;
        } else {
            pending = false;
            notified = -1;
            if (!stopping) rearm_listen();
        }
        return;
    }
    // 没有活着的子进程：先不 accept，连接留在 backlog 里等新子进程。
    if (!stopping) rearm_listen();
}

void handle_ack(int fd) {
    char byte = 0;
    while (read(fd, &byte, 1) > 0) {
    }
    pending = false;
    notified = -1;
    if (!stopping) rearm_listen();
}

void handle_sigchld() {
    for (;;) {
        int status = 0;
        const pid_t pid = waitpid(-1, &status, WNOHANG);
        if (pid <= 0) return;
        int index = -1;
        for (int i = 0; i < child_count; ++i) {
            if (children[i].pid == pid) index = i;
        }
        if (index < 0) continue;
        if (children[index].parent_fd >= 0) {
            epoll_ctl(epfd, EPOLL_CTL_DEL, children[index].parent_fd, nullptr);
            close(children[index].parent_fd);
        }
        children[index].pid = -1;
        children[index].parent_fd = -1;
        std::printf("父进程: 回收子进程 pid=%d，存活 %d 个\n", static_cast<int>(pid),
                    live_children());
        std::fflush(stdout);
        if (pending && notified == index) {
            pending = false;
            notified = -1;
        }
        if (!stopping) {
            spawn_child(index);
            std::printf("父进程: 补一个子进程，存活 %d 个\n", live_children());
            std::fflush(stdout);
        }
        if (!pending && !stopping) rearm_listen();
    }
}

void handle_signal(int fd) {
    signalfd_siginfo info{};
    while (read(fd, &info, sizeof info) == sizeof info) {
        if (info.ssi_signo == SIGCHLD) {
            handle_sigchld();
        } else if (info.ssi_signo == SIGTERM || info.ssi_signo == SIGINT) {
            stopping = true;
        }
    }
}

void shutdown_pool() {
    stopping = true;
    epoll_ctl(epfd, EPOLL_CTL_DEL, listen_fd, nullptr);
    close(listen_fd);
    listen_fd = -1;
    for (int i = 0; i < child_count; ++i) {
        if (children[i].pid > 0) kill(children[i].pid, SIGTERM);
    }
    for (;;) {
        const pid_t pid = waitpid(-1, nullptr, 0);
        if (pid < 0) {
            if (errno == EINTR) continue;
            break;
        }
    }
    for (int i = 0; i < child_count; ++i) {
        if (children[i].parent_fd >= 0) close(children[i].parent_fd);
        children[i].parent_fd = -1;
        children[i].pid = -1;
    }
    std::printf("processpool: stopped\n");
    std::fflush(stdout);
}

}  // namespace

int main(int argc, char **argv) {
    std::uint16_t port = 0;
    const char *port_file = nullptr;
    child_count = 4;
    for (int i = 1; i < argc; ++i) {
        if (std::strcmp(argv[i], "--port") == 0 && i + 1 < argc) {
            port = parse_port(argv[++i]);
        } else if (std::strcmp(argv[i], "--port-file") == 0 && i + 1 < argc) {
            port_file = argv[++i];
        } else if (std::strcmp(argv[i], "--children") == 0 && i + 1 < argc) {
            child_count = std::atoi(argv[++i]);
        } else {
            std::fprintf(stderr,
                         "usage: %s (--port P | --port-file F) [--children N]\n",
                         argv[0]);
            return 2;
        }
    }
    if (child_count < 1) child_count = 1;
    if (child_count > kMaxChildren) child_count = kMaxChildren;

    sigset_t mask;
    sigemptyset(&mask);
    sigaddset(&mask, SIGCHLD);
    sigaddset(&mask, SIGTERM);
    sigaddset(&mask, SIGINT);
    if (pthread_sigmask(SIG_BLOCK, &mask, nullptr) != 0) return 1;

    try {
        listen_fd = listen_tcp(port, port_file);
        set_nonblocking(listen_fd);  // 子进程的 accept 循环要读到 EAGAIN
        // 先建 epoll，spawn_child 里要把父进程这一端的 socketpair 加进去；
        // fork 出的子进程会继承它，child_cleanup 会关掉。
        epfd = epoll_create1(EPOLL_CLOEXEC);
        if (epfd < 0) throw std::system_error(errno, std::generic_category(), "epoll_create1");
        for (int i = 0; i < child_count; ++i) spawn_child(i);
        add_fd(listen_fd, EPOLLIN | EPOLLONESHOT);
        sig_fd = signalfd(-1, &mask, SFD_NONBLOCK | SFD_CLOEXEC);
        if (sig_fd < 0) throw std::system_error(errno, std::generic_category(), "signalfd");
        add_fd(sig_fd, EPOLLIN);
    } catch (const std::exception &e) {
        std::fprintf(stderr, "processpool: %s\n", e.what());
        return 1;
    }

    std::printf("processpool: started children=%d\n", child_count);
    std::fflush(stdout);

    epoll_event events[16];
    while (!stopping) {
        const int n = epoll_wait(epfd, events, 16, 200);
        if (n < 0) {
            if (errno == EINTR) continue;
            break;
        }
        for (int i = 0; i < n; ++i) {
            const int fd = events[i].data.fd;
            if (fd == listen_fd) {
                notify_next();
            } else if (fd == sig_fd) {
                handle_signal(fd);
            } else {
                handle_ack(fd);
            }
        }
    }
    shutdown_pool();
    close(epfd);
    close(sig_fd);
    return 0;
}
