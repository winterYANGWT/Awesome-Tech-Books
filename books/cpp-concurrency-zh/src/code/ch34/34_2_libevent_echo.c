// 单线程 Libevent 回射：监听 fd、两条连接、两个客户端都跑在同一个 base 里。
// flags: -levent
// 连接用 EV_READ|EV_PERSIST；有积压输出时才把 EV_WRITE 事件加进来，避免可写事件空转。
#include <event2/event.h>

#include <arpa/inet.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_CONNS 16

struct Conn {
    int fd;
    char out[8192];
    size_t out_len;
    int eof;
    struct event *rev;
    struct event *wev;
};

struct Client {
    int fd;
    const char *payload;
    size_t got;
    struct event *rev;
};

static struct event_base *base;
static struct event *listener_ev;
static struct Conn *conns[MAX_CONNS];
static int conn_count;
static unsigned long long served;
static unsigned long long echoed;
static int done_clients;
static size_t client_bytes;

static void close_conn(struct Conn *c) {
    if (c->fd < 0) return;
    event_del(c->rev);
    event_del(c->wev);
    close(c->fd);
    c->fd = -1;
}

static void flush_out(struct Conn *c) {
    while (c->out_len > 0) {
        ssize_t n = send(c->fd, c->out, c->out_len, MSG_NOSIGNAL);
        if (n > 0) {
            memmove(c->out, c->out + n, c->out_len - (size_t)n);
            c->out_len -= (size_t)n;
            continue;
        }
        if (n < 0 && errno == EINTR) continue;
        if (n < 0 && (errno == EAGAIN || errno == EWOULDBLOCK)) break;
        close_conn(c);
        return;
    }
    if (c->out_len == 0) {
        event_del(c->wev);
        if (c->eof) close_conn(c);
    } else {
        event_add(c->wev, NULL);
    }
}

static void on_conn_write(evutil_socket_t fd, short what, void *arg) {
    (void)fd;
    (void)what;
    flush_out((struct Conn *)arg);
}

static void on_conn_read(evutil_socket_t fd, short what, void *arg) {
    (void)what;
    struct Conn *c = (struct Conn *)arg;
    char buf[1024];
    for (;;) {
        ssize_t n = recv(fd, buf, sizeof buf, 0);
        if (n > 0) {
            if (c->out_len + (size_t)n > sizeof c->out) {
                close_conn(c);
                return;
            }
            memcpy(c->out + c->out_len, buf, (size_t)n);
            c->out_len += (size_t)n;
            echoed += (unsigned long long)n;
            flush_out(c);
            if (c->fd < 0) return;
            continue;
        }
        if (n == 0) {
            c->eof = 1;
            event_del(c->rev);
            if (c->out_len == 0) close_conn(c);
            return;
        }
        if (errno == EINTR) continue;
        if (errno == EAGAIN || errno == EWOULDBLOCK) return;
        close_conn(c);
        return;
    }
}

static void on_accept(evutil_socket_t lfd, short what, void *arg) {
    (void)what;
    (void)arg;
    for (;;) {
        int fd = accept4(lfd, NULL, NULL, SOCK_NONBLOCK | SOCK_CLOEXEC);
        if (fd < 0) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) return;
            if (errno == EINTR) continue;
            return;
        }
        if (conn_count == MAX_CONNS) {
            close(fd);
            continue;
        }
        struct Conn *c = (struct Conn *)calloc(1, sizeof *c);
        if (c == NULL) {
            close(fd);
            continue;
        }
        c->fd = fd;
        c->rev = event_new(base, fd, EV_READ | EV_PERSIST, on_conn_read, c);
        c->wev = event_new(base, fd, EV_WRITE | EV_PERSIST, on_conn_write, c);
        event_add(c->rev, NULL);
        conns[conn_count++] = c;
        ++served;
    }
}

static void on_client_read(evutil_socket_t fd, short what, void *arg) {
    (void)what;
    struct Client *c = (struct Client *)arg;
    char buf[256];
    for (;;) {
        ssize_t n = recv(fd, buf, sizeof buf, 0);
        if (n > 0) {
            c->got += (size_t)n;
            if (c->got >= strlen(c->payload)) {
                event_del(c->rev);
                close(fd);
                c->fd = -1;
                ++done_clients;
                client_bytes += c->got;
                if (done_clients == 2) event_base_loopexit(base, NULL);
                return;
            }
            continue;
        }
        if (n == 0) return;
        if (errno == EINTR) continue;
        if (errno == EAGAIN || errno == EWOULDBLOCK) return;
        return;
    }
}

static int connect_client(const char *payload, struct Client *c) {
    int fd = socket(AF_INET, SOCK_STREAM | SOCK_CLOEXEC, 0);
    if (fd < 0) return -1;
    struct sockaddr_in addr;
    memset(&addr, 0, sizeof addr);
    socklen_t len = sizeof addr;
    if (getsockname(event_get_fd(listener_ev), (struct sockaddr *)&addr, &len) != 0) {
        close(fd);
        return -1;
    }
    if (connect(fd, (struct sockaddr *)&addr, sizeof addr) != 0) {
        close(fd);
        return -1;
    }
    c->fd = fd;
    c->payload = payload;
    c->got = 0;
    size_t sent = 0;
    size_t total = strlen(payload);
    while (sent < total) {
        ssize_t n = send(fd, payload + sent, total - sent, MSG_NOSIGNAL);
        if (n > 0) {
            sent += (size_t)n;
        } else if (n < 0 && errno == EINTR) {
            continue;
        } else {
            close(fd);
            return -1;
        }
    }
    int flags = fcntl(fd, F_GETFL, 0);
    if (flags < 0 || fcntl(fd, F_SETFL, flags | O_NONBLOCK) < 0) {
        close(fd);
        return -1;
    }
    c->rev = event_new(base, fd, EV_READ | EV_PERSIST, on_client_read, c);
    event_add(c->rev, NULL);
    return 0;
}

int main(void) {
    int lfd = socket(AF_INET, SOCK_STREAM | SOCK_CLOEXEC, 0);
    if (lfd < 0) return 1;
    struct sockaddr_in addr;
    memset(&addr, 0, sizeof addr);
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    addr.sin_port = 0;
    if (bind(lfd, (struct sockaddr *)&addr, sizeof addr) != 0) return 1;
    if (listen(lfd, 16) != 0) return 1;
    int lflags = fcntl(lfd, F_GETFL, 0);
    if (lflags < 0 || fcntl(lfd, F_SETFL, lflags | O_NONBLOCK) < 0) return 1;

    base = event_base_new();
    if (base == NULL) return 1;
    listener_ev = event_new(base, lfd, EV_READ | EV_PERSIST, on_accept, NULL);
    event_add(listener_ev, NULL);

    struct Client c1;
    struct Client c2;
    if (connect_client("hello-one", &c1) != 0) return 1;
    if (connect_client("hello-two", &c2) != 0) return 1;

    int rc = event_base_dispatch(base);
    printf("clients: done=%d received=%zu\n", done_clients, client_bytes);
    printf("server: served=%llu echoed=%llu\n", served, echoed);
    printf("loop: rc=%d\n", rc);

    event_free(c1.rev);
    event_free(c2.rev);
    if (c1.fd >= 0) close(c1.fd);
    if (c2.fd >= 0) close(c2.fd);
    for (int i = 0; i < conn_count; ++i) {
        struct Conn *c = conns[i];
        if (c->fd >= 0) close(c->fd);
        event_free(c->rev);
        event_free(c->wev);
        free(c);
    }
    event_free(listener_ev);
    close(lfd);
    event_base_free(base);
    return 0;
}
