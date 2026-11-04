// mode: run
// 对比 long 和 intptr_t 作为 void* 参数：先看宽度，再看往返结果。
#define _POSIX_C_SOURCE 200809L
#include <pthread.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#define CHECK(call)                                                            \
    do {                                                                       \
        int rc_ = (call);                                                      \
        if (rc_ != 0) {                                                        \
            fprintf(stderr, "%s: %s\n", #call, strerror(rc_));                 \
            return 1;                                                          \
        }                                                                      \
    } while (0)

struct round_trip {
    long original_long;
    intptr_t original_intptr;
    long back_long;
    intptr_t back_intptr;
};

static void* worker(void* arg) {
    struct round_trip* value = arg;
    // 两种写法都先把整数转成 void*，再在入口里转回来。
    value->back_long = (long)(void*)value->original_long;
    value->back_intptr = (intptr_t)(void*)value->original_intptr;
    return NULL;
}

int main(void) {
    printf("sizeof(void*) = %zu, sizeof(long) = %zu, sizeof(intptr_t) = %zu\n",
           sizeof(void*), sizeof(long), sizeof(intptr_t));
    printf("本机是指针和 long 等宽的 LP64：%s\n",
           sizeof(void*) == sizeof(long) ? "是" : "否");

    struct round_trip value = {.original_long = 123456789,
                               .original_intptr = 123456789,
                               .back_long = 0,
                               .back_intptr = 0};
    pthread_t thread;
    CHECK(pthread_create(&thread, NULL, worker, &value));
    CHECK(pthread_join(thread, NULL));

    printf("long 往返：%ld -> %ld（%s）\n", value.original_long, value.back_long,
           value.original_long == value.back_long ? "一致" : "不一致");
    printf("intptr_t 往返：%ld -> %ld（%s）\n", (long)value.original_intptr,
           (long)value.back_intptr,
           value.original_intptr == value.back_intptr ? "一致" : "不一致");

    long address_as_long = (long)(intptr_t)&value;
    void* address_back = (void*)(intptr_t)address_as_long;
    printf("把地址存进 long 再转回：%s\n",
           address_back == (void*)&value ? "一致" : "不一致");
    return 0;
}
