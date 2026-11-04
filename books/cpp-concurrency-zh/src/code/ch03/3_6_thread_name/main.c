// mode: run
// 两个工作线程各自命名；main 等名字设好后再通知 example.sh 去读 /proc。
#define _GNU_SOURCE
#include <pthread.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#define CHECK(call)                                                            \
    do {                                                                       \
        int rc_ = (call);                                                      \
        if (rc_ != 0) {                                                        \
            fprintf(stderr, "%s: %s\n", #call, strerror(rc_));                 \
            return 1;                                                          \
        }                                                                      \
    } while (0)

static pthread_barrier_t ready_barrier;

static void* worker(void* arg) {
    const char* name = arg;
    int rc = pthread_setname_np(pthread_self(), name);
    if (rc != 0) {
        fprintf(stderr, "pthread_setname_np(%s): %s\n", name, strerror(rc));
        return NULL;
    }

    pthread_barrier_wait(&ready_barrier);

    struct timespec pause = {.tv_sec = 5, .tv_nsec = 0};
    nanosleep(&pause, NULL);
    return NULL;
}

int main(int argc, char** argv) {
    if (argc != 2) {
        fprintf(stderr, "用法: %s <ready 文件>\n", argv[0]);
        return 2;
    }

    CHECK(pthread_barrier_init(&ready_barrier, NULL, 3));

    pthread_t reader;
    pthread_t writer;
    CHECK(pthread_create(&reader, NULL, worker, "reader-0"));
    CHECK(pthread_create(&writer, NULL, worker, "writer-1"));

    pthread_barrier_wait(&ready_barrier);

    FILE* file = fopen(argv[1], "w");
    if (file == NULL) {
        return 3;
    }
    fprintf(file, "ready\n");
    fclose(file);

    CHECK(pthread_join(reader, NULL));
    CHECK(pthread_join(writer, NULL));
    CHECK(pthread_barrier_destroy(&ready_barrier));
    printf("两个工作线程已汇入\n");
    return 0;
}
