// mode: run
// 打印 pthread_attr_t 的默认值，再改栈大小和保护区。
#define _POSIX_C_SOURCE 200809L
#include <limits.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/resource.h>
#include <unistd.h>

#define CHECK(call)                                                            \
    do {                                                                       \
        int rc_ = (call);                                                      \
        if (rc_ != 0) {                                                        \
            fprintf(stderr, "%s: %s\n", #call, strerror(rc_));                 \
            exit(1);                                                           \
        }                                                                      \
    } while (0)

static const char* detach_name(int value) {
    if (value == PTHREAD_CREATE_JOINABLE) {
        return "PTHREAD_CREATE_JOINABLE";
    }
    if (value == PTHREAD_CREATE_DETACHED) {
        return "PTHREAD_CREATE_DETACHED";
    }
    return "未知";
}

static const char* policy_name(int value) {
    switch (value) {
        case SCHED_OTHER:
            return "SCHED_OTHER";
        case SCHED_FIFO:
            return "SCHED_FIFO";
        case SCHED_RR:
            return "SCHED_RR";
        default:
            return "未知";
    }
}

static const char* inherit_name(int value) {
    if (value == PTHREAD_INHERIT_SCHED) {
        return "PTHREAD_INHERIT_SCHED";
    }
    if (value == PTHREAD_EXPLICIT_SCHED) {
        return "PTHREAD_EXPLICIT_SCHED";
    }
    return "未知";
}

static void print_attr(const char* tag, const pthread_attr_t* attr) {
    size_t stack_size = 0;
    size_t guard_size = 0;
    int detach = 0;
    int policy = 0;
    int inherit = 0;
    int scope = 0;
    CHECK(pthread_attr_getstacksize(attr, &stack_size));
    CHECK(pthread_attr_getguardsize(attr, &guard_size));
    CHECK(pthread_attr_getdetachstate(attr, &detach));
    CHECK(pthread_attr_getschedpolicy(attr, &policy));
    CHECK(pthread_attr_getinheritsched(attr, &inherit));
    CHECK(pthread_attr_getscope(attr, &scope));

    printf("%s\n", tag);
    printf("  stacksize   = %zu 字节（%zu KiB）\n", stack_size, stack_size / 1024);
    printf("  guardsize   = %zu 字节\n", guard_size);
    printf("  detachstate = %s\n", detach_name(detach));
    printf("  schedpolicy = %s\n", policy_name(policy));
    printf("  inheritsched= %s\n", inherit_name(inherit));
    printf("  scope       = %s\n",
           scope == PTHREAD_SCOPE_SYSTEM ? "PTHREAD_SCOPE_SYSTEM"
                                         : "PTHREAD_SCOPE_PROCESS");
}

int main(void) {
    struct rlimit limit;
    CHECK(getrlimit(RLIMIT_STACK, &limit));
    printf("sizeof(pthread_attr_t) = %zu\n", sizeof(pthread_attr_t));
    printf("PTHREAD_STACK_MIN = %ld 字节，页大小 = %ld 字节\n",
           (long)PTHREAD_STACK_MIN, (long)sysconf(_SC_PAGESIZE));
    printf("RLIMIT_STACK 软限制 = %llu 字节\n",
           (unsigned long long)limit.rlim_cur);
    printf("线程栈默认值取自进程启动时的 RLIMIT_STACK\n");

    pthread_attr_t attr;
    CHECK(pthread_attr_init(&attr));
    print_attr("默认属性：", &attr);

    int rc = pthread_attr_setscope(&attr, PTHREAD_SCOPE_PROCESS);
    printf("setscope(PTHREAD_SCOPE_PROCESS) = %d（%s）\n", rc, strerror(rc));

    CHECK(pthread_attr_setstacksize(&attr, 1024 * 1024));
    CHECK(pthread_attr_setguardsize(&attr, 8 * 1024));
    print_attr("设置 1 MiB 栈和 8 KiB 保护区之后：", &attr);

    CHECK(pthread_attr_destroy(&attr));
    return 0;
}
