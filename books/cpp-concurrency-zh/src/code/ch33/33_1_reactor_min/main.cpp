#include "acceptor.hpp"
#include "connection.hpp"
#include "event_loop.hpp"

#include "../../kvd/net.hpp"

#include <signal.h>
#include <sys/epoll.h>
#include <unistd.h>

#include <cstdio>
#include <cstring>

int main(int argc, char **argv) {
    const char *port_file = nullptr;
    for (int i = 1; i < argc; ++i) {
        if (std::strcmp(argv[i], "--port-file") == 0 && i + 1 < argc) {
            port_file = argv[++i];
        }
    }
    if (port_file == nullptr) {
        std::fprintf(stderr, "usage: %s --port-file FILE\n", argv[0]);
        return 2;
    }

    std::setvbuf(stdout, nullptr, _IOLBF, 0);
    ::signal(SIGPIPE, SIG_IGN);

    const int listen_fd = listen_tcp(0, port_file);
    set_nonblocking(listen_fd);

    EventLoop loop;
    ConnectionManager manager(loop);
    Acceptor acceptor(loop, listen_fd, manager);
    loop.add(&acceptor, EPOLLIN);

    std::printf("server: listening (idle timeout 3s)\n");
    loop.run();

    ::close(listen_fd);
    return 0;
}
