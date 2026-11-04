// mode: run
// flags: -levent -levent_pthreads
// expect: method = epoll
// expect: callback: 10 ms
// 新写法（Libevent 2.0 起）：先开线程支持，再 event_base_new；三个定时器按到期时间回调。
// 旧写法 event_init() 会设置全局 current_base，已弃用，只在注释里给出等价代码。
#define _GNU_SOURCE
#include <event2/event.h>
#include <event2/thread.h>
#include <stdio.h>

static void on_timer(evutil_socket_t fd, short what, void *arg) {
    (void)fd;
    (void)what;
    printf("callback: %s\n", (const char *)arg);
}

int main(void) {
    // 必须在创建 event_base 之前调用；否则线程支持不会装到 base 上。
    int ret = evthread_use_pthreads();
    printf("evthread_use_pthreads = %d\n", ret);

    // 旧写法：struct event_base *base = event_init();
    // event_init 返回全局 current_base，同进程只能有一个，且不是线程安全的。
    struct event_base *base = event_base_new();
    if (!base) {
        fprintf(stderr, "event_base_new failed\n");
        return 1;
    }
    printf("method = %s\n", event_base_get_method(base));

    struct event *t10 = evtimer_new(base, on_timer, (void *)"10 ms");
    struct event *t30 = evtimer_new(base, on_timer, (void *)"30 ms");
    struct event *t60 = evtimer_new(base, on_timer, (void *)"60 ms");
    struct timeval d10 = {.tv_sec = 0, .tv_usec = 10000};
    struct timeval d30 = {.tv_sec = 0, .tv_usec = 30000};
    struct timeval d60 = {.tv_sec = 0, .tv_usec = 60000};
    event_add(t60, &d60);
    event_add(t10, &d10);
    event_add(t30, &d30);

    ret = event_base_dispatch(base);
    printf("dispatch = %d (no events left)\n", ret);

    event_free(t10);
    event_free(t30);
    event_free(t60);
    event_base_free(base);
    return 0;
}
