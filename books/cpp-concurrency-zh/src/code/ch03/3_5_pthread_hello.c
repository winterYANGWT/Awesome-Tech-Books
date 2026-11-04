// mode: run
// args: 4
// output: varies
// expect: 全部 4 个线程已汇入，退出值之和：130
// 线程入口通过 intptr_t 收编号，退出值通过 pthread_join 的 void** 取回。
#define _POSIX_C_SOURCE 200809L
#include <pthread.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define CHECK(call)                                                            \
    do {                                                                       \
        int rc_ = (call);                                                      \
        if (rc_ != 0) {                                                        \
            fprintf(stderr, "%s: %s\n", #call, strerror(rc_));                 \
            exit(1);                                                           \
        }                                                                      \
    } while (0)

static void* worker(void* arg) {
    intptr_t id = (intptr_t)arg;
    printf("线程 %ld 启动\n", (long)id);
    if (id == 0) {
        pthread_exit((void*)(intptr_t)70);  // 显式结束当前线程，退出值交给 join 者
    }
    return (void*)(id * 10);                // 正常 return 与 pthread_exit 等价
}

int main(int argc, char** argv) {
    if (argc != 2) {
        fprintf(stderr, "用法: %s <线程数>\n", argv[0]);
        return 2;
    }
    char* end = NULL;
    long count = strtol(argv[1], &end, 10);
    if (end == argv[1] || *end != '\0' || count < 1 || count > 64) {
        fprintf(stderr, "线程数必须在 1..64 之间\n");
        return 2;
    }

    pthread_t* tids = calloc((size_t)count, sizeof(*tids));
    if (tids == NULL) {
        return 1;
    }

    for (intptr_t i = 0; i < (intptr_t)count; ++i) {
        CHECK(pthread_create(&tids[i], NULL, worker, (void*)i));
    }

    long sum = 0;
    for (long i = 0; i < count; ++i) {
        void* returned = NULL;
        CHECK(pthread_join(tids[i], &returned));
        long value = (long)(intptr_t)returned;
        sum += value;
        printf("主线程收到线程 %ld 的退出值：%ld\n", i, value);
    }
    printf("全部 %ld 个线程已汇入，退出值之和：%ld\n", count, sum);
    free(tids);
    return 0;
}
