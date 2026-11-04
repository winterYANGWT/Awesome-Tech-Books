// mode: hang
// expect: (pthread_mutex_lock|强制终止)
// NORMAL 锁在同一线程重复加锁不是错误码，而是直接挂起。
#define _POSIX_C_SOURCE 200809L

#include <pthread.h>
#include <stdio.h>

int main(void) {
    pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
    (void)pthread_mutex_lock(&mutex);
    printf("第一次 lock 返回，锁归当前线程\n");
    fflush(stdout);
    (void)pthread_mutex_lock(&mutex);  // 第二个 lock 永远不返回
    printf("第二次 lock 返回（不会到这里）\n");
    return 0;
}
