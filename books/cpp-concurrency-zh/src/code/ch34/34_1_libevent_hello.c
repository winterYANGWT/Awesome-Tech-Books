// Libevent 的最小循环：一个监听 fd、一个定时器、一个信号，全部进同一个 base。
// flags: -levent
// 定时器触发后自己 raise(SIGINT)，这样不用外部按键也能演示信号事件和退出顺序。
#include <event2/event.h>

#include <arpa/inet.h>
#include <netinet/in.h>
#include <signal.h>
#include <sys/socket.h>
#include <unistd.h>

#include <stdio.h>
#include <string.h>

static struct event_base *base;

static void on_signal(evutil_socket_t sig, short what, void *arg) {
    (void)sig;
    (void)what;
    (void)arg;
    printf("2 SIGINT：信号事件，准备退出\n");
    event_base_loopexit(base, NULL);
}

static void on_timer(evutil_socket_t fd, short what, void *arg) {
    (void)fd;
    (void)what;
    (void)arg;
    printf("1 timer：定时器先到，raise(SIGINT)\n");
    raise(SIGINT);
}

static void on_accept(evutil_socket_t fd, short what, void *arg) {
    (void)fd;
    (void)what;
    (void)arg;
    printf("listener：本示例不接连接\n");
}

static const char *feature_names(enum event_method_feature f, char *buf, size_t n) {
    buf[0] = '\0';
    if (f & EV_FEATURE_ET) strncat(buf, "ET", n - strlen(buf) - 1);
    if (f & EV_FEATURE_O1) {
        if (buf[0] != '\0') strncat(buf, "|", n - strlen(buf) - 1);
        strncat(buf, "O1", n - strlen(buf) - 1);
    }
    if (f & EV_FEATURE_FDS) {
        if (buf[0] != '\0') strncat(buf, "|", n - strlen(buf) - 1);
        strncat(buf, "FDS", n - strlen(buf) - 1);
    }
    return buf;
}

int main(void) {
    printf("libevent %s\n", event_get_version());

    base = event_base_new();
    if (base == NULL) return 1;
    char feats[32];
    printf("method: %s\n", event_base_get_method(base));
    printf("features: %s\n",
           feature_names(event_base_get_features(base), feats, sizeof feats));

    int lfd = socket(AF_INET, SOCK_STREAM | SOCK_CLOEXEC, 0);
    if (lfd < 0) return 1;
    struct sockaddr_in addr;
    memset(&addr, 0, sizeof addr);
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    addr.sin_port = 0;  // 端口由内核选；本示例不打印，输出才能确定
    if (bind(lfd, (struct sockaddr *)&addr, sizeof addr) != 0) return 1;
    if (listen(lfd, 16) != 0) return 1;
    struct event *listener = event_new(base, lfd, EV_READ | EV_PERSIST, on_accept, NULL);
    event_add(listener, NULL);
    printf("listener: registered\n");

    struct event *timer = evtimer_new(base, on_timer, NULL);
    struct timeval tv = {0, 100000};  // 100 ms
    event_add(timer, &tv);

    struct event *sig = evsignal_new(base, SIGINT, on_signal, NULL);
    event_add(sig, NULL);

    int rc = event_base_dispatch(base);
    printf("3 loop: rc=%d got_exit=%d\n", rc, event_base_got_exit(base));

    event_free(sig);
    event_free(timer);
    event_free(listener);
    close(lfd);
    event_base_free(base);

    // 没有任何 pending/active 事件时，dispatch 立即返回 1。
    struct event_base *empty = event_base_new();
    int rc_empty = event_base_dispatch(empty);
    printf("empty base: rc=%d\n", rc_empty);
    event_base_free(empty);
    return 0;
}
