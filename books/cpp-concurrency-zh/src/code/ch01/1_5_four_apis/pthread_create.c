// mode: run
// output: varies
// expect: pthread 执行流
#define _POSIX_C_SOURCE 200809L
#include <pthread.h>
#include <stdio.h>

static void* worker(void* arg) {
    const int id = *(const int*)arg;
    printf("pthread 执行流 %d/4\n", id);
    return NULL;
}

int main(void) {
    // ANCHOR: core
    pthread_t threads[4];
    int ids[4];
    for (int i = 0; i < 4; ++i) {
        ids[i] = i;
        const int rc = pthread_create(&threads[i], NULL, worker, &ids[i]);
        if (rc != 0) {
            fprintf(stderr, "pthread_create: %d\n", rc);
            return 1;
        }
    }
    for (int i = 0; i < 4; ++i) {
        const int rc = pthread_join(threads[i], NULL);
        if (rc != 0) {
            fprintf(stderr, "pthread_join: %d\n", rc);
            return 1;
        }
    }
    // ANCHOR_END: core
    return 0;
}
