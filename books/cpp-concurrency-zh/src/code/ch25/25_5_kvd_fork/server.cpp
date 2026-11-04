// kvd_fork：父进程只 accept，每个连接 fork 一个子进程；子进程有自己的 store 副本。
#include "net.hpp"
#include "protocol.hpp"
#include "store.hpp"

#include <signal.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <unistd.h>

#include <cerrno>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <string_view>

namespace {

volatile sig_atomic_t g_stop = 0;
volatile sig_atomic_t g_reaped = 0;
volatile sig_atomic_t g_lfd = -1;
volatile sig_atomic_t g_child_slot[64];

void on_term(int) {
    g_stop = 1;
    // shutdown 是 async-signal-safe 的；它能让阻塞在 accept 上的父进程醒过来。
    if (g_lfd >= 0) {
        shutdown(static_cast<int>(g_lfd), SHUT_RDWR);
    }
}

void on_sigchld(int) {
    int saved = errno;
    for (;;) {
        int status = 0;
        pid_t pid = waitpid(-1, &status, WNOHANG);
        if (pid <= 0) {
            break;
        }
        g_reaped = static_cast<sig_atomic_t>(g_reaped + 1);
        for (int i = 0; i < 64; ++i) {
            if (g_child_slot[i] == pid) {
                g_child_slot[i] = -1;
            }
        }
    }
    errno = saved;
}

volatile sig_atomic_t g_child_stop = 0;
volatile sig_atomic_t g_cfd = -1;

void on_child_term(int) {
    g_child_stop = 1;
    if (g_cfd >= 0) {
        close(static_cast<int>(g_cfd));  // 让阻塞在 recv 上的子进程返回
        g_cfd = -1;
    }
}

void serve(int cfd, Store *store) {
    Parser parser;
    char buf[4096];
    while (!g_child_stop) {
        ssize_t n = recv(cfd, buf, sizeof buf, 0);
        if (n < 0) {
            if (errno == EINTR) {
                continue;
            }
            break;
        }
        if (n == 0) {
            break;
        }
        parser.feed(std::string_view(buf, static_cast<std::size_t>(n)));
        bool failed = false;
        while (auto result = parser.next()) {
            std::string out;
            if (auto *req = std::get_if<Request>(&*result)) {
                out = format_response(store->apply(*req));
            } else {
                out = format_response(std::get<ParseError>(*result));
            }
            try {
                if (!write_all(cfd, out)) {
                    failed = true;
                    break;
                }
            } catch (...) {
                failed = true;
                break;
            }
        }
        if (failed) {
            break;
        }
    }
}

std::uint16_t parse_port(const char *text) {
    char *end = nullptr;
    unsigned long v = std::strtoul(text, &end, 10);
    if (end == text || *end != '\0' || v > 65535) {
        std::fprintf(stderr, "bad port: %s\n", text);
        std::exit(2);
    }
    return static_cast<std::uint16_t>(v);
}

void wait_for_children() {
    for (;;) {
        pid_t pid = waitpid(-1, nullptr, 0);
        if (pid > 0) {
            g_reaped = static_cast<sig_atomic_t>(g_reaped + 1);
            continue;
        }
        if (pid < 0 && errno == EINTR) {
            continue;
        }
        break;
    }
}

}  // namespace

int main(int argc, char **argv) {
    std::uint16_t port = 0;
    const char *port_file = nullptr;
    for (int i = 1; i < argc; ++i) {
        if (std::strcmp(argv[i], "--port") == 0 && i + 1 < argc) {
            port = parse_port(argv[++i]);
        } else if (std::strcmp(argv[i], "--port-file") == 0 && i + 1 < argc) {
            port_file = argv[++i];
        }
    }
    if (port == 0 && port_file == nullptr) {
        std::fprintf(stderr, "usage: %s (--port P | --port-file F)\n", argv[0]);
        return 2;
    }
    for (int i = 0; i < 64; ++i) {
        g_child_slot[i] = -1;
    }

    struct sigaction sa;
    std::memset(&sa, 0, sizeof sa);
    sa.sa_handler = on_term;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;  // 不重启 accept，SIGTERM 才能让它返回 EINTR
    sigaction(SIGTERM, &sa, nullptr);
    sigaction(SIGINT, &sa, nullptr);

    sa.sa_handler = on_sigchld;
    sa.sa_flags = SA_RESTART;
    sigaction(SIGCHLD, &sa, nullptr);
    signal(SIGPIPE, SIG_IGN);

    g_lfd = listen_tcp(port, port_file);
    // 在 fork 前建好，子进程拿到的是它的副本；父进程自己不用它。
    Store store(16);
    std::uint64_t connections = 0;

// ANCHOR: accept_fork
    while (!g_stop) {
        int cfd = accept(static_cast<int>(g_lfd), nullptr, nullptr);
        if (cfd < 0) {
            if (errno == EINTR) {
                continue;
            }
            if (g_stop) {
                break;
            }
            std::perror("accept");
            break;
        }
        ++connections;

        // 从 fork 到登记完 PID 之间挡住 SIGCHLD，免得处理函数先回收、父进程再登记一个失效的 PID。
        sigset_t chld_mask;
        sigset_t old_mask;
        sigemptyset(&chld_mask);
        sigaddset(&chld_mask, SIGCHLD);
        sigprocmask(SIG_BLOCK, &chld_mask, &old_mask);

        pid_t pid = fork();
        if (pid == 0) {
            sigprocmask(SIG_SETMASK, &old_mask, nullptr);
            // 子进程不 accept，先关掉监听 fd，否则端口永远不释放。
            close(static_cast<int>(g_lfd));
            g_lfd = -1;

            struct sigaction child_sa;
            std::memset(&child_sa, 0, sizeof child_sa);
            child_sa.sa_handler = on_child_term;
            sigemptyset(&child_sa.sa_mask);
            sigaction(SIGTERM, &child_sa, nullptr);
            sigaction(SIGINT, &child_sa, nullptr);

            g_cfd = cfd;
            serve(cfd, &store);  // 用的是 fork 时复制出来的那一份
            _exit(0);
        }

        if (pid < 0) {
            std::perror("fork");
            close(cfd);
            sigprocmask(SIG_SETMASK, &old_mask, nullptr);
            continue;
        }
        for (int i = 0; i < 64; ++i) {
            if (g_child_slot[i] < 0) {
                g_child_slot[i] = pid;
                break;
            }
        }
        sigprocmask(SIG_SETMASK, &old_mask, nullptr);
        close(cfd);  // 父进程不留连接 fd
    }
// ANCHOR_END: accept_fork

    if (g_lfd >= 0) {
        close(static_cast<int>(g_lfd));
    }

    // 先挡住 SIGCHLD，避免处理函数和下面的 kill/waitpid 抢同一个子进程。
    sigset_t chld;
    sigset_t old;
    sigemptyset(&chld);
    sigaddset(&chld, SIGCHLD);
    sigprocmask(SIG_BLOCK, &chld, &old);
    for (int i = 0; i < 64; ++i) {
        if (g_child_slot[i] > 0) {
            kill(static_cast<pid_t>(g_child_slot[i]), SIGTERM);
        }
    }
    wait_for_children();
    sigprocmask(SIG_SETMASK, &old, nullptr);

    std::printf("server: connections=%llu reaped=%d\n",
                (unsigned long long)connections, static_cast<int>(g_reaped));
    return 0;
}
