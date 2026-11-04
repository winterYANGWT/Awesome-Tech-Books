// mode: run
// tsan: no（示例故意做非持有者 unlock，TSan 会把它当成错误报告）
// expect: ERRORCHECK：lock 0，重复 lock EDEADLK，非持有者 unlock EPERM，重复 unlock EPERM
#define _POSIX_C_SOURCE 200809L

#include <errno.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static const char *err_name(int rc) {
    switch (rc) {
    case 0: return "0";
    case EDEADLK: return "EDEADLK";
    case EPERM: return "EPERM";
    case EBUSY: return "EBUSY";
    case EINVAL: return "EINVAL";
    default: return "其他错误";
    }
}

#define CHECK(call)                                                     \
    do {                                                                \
        const int rc_ = (call);                                         \
        if (rc_ != 0) {                                                 \
            fprintf(stderr, "%s 失败：%s\n", #call, strerror(rc_));     \
            exit(EXIT_FAILURE);                                         \
        }                                                               \
    } while (0)

struct unlock_arg {
    pthread_mutex_t *mutex;
    int rc;
};

static void *unlock_from_other_thread(void *p) {
    struct unlock_arg *arg = p;
    arg->rc = pthread_mutex_unlock(arg->mutex);
    return NULL;
}

static int unlock_by_other(pthread_mutex_t *mutex) {
    struct unlock_arg arg = {mutex, 0};
    pthread_t thread;
    CHECK(pthread_create(&thread, NULL, unlock_from_other_thread, &arg));
    CHECK(pthread_join(thread, NULL));
    return arg.rc;
}

int main(void) {
    pthread_mutex_t normal = PTHREAD_MUTEX_INITIALIZER;
    CHECK(pthread_mutex_lock(&normal));
    const int normal_try = pthread_mutex_trylock(&normal);
    CHECK(pthread_mutex_unlock(&normal));
    printf("NORMAL：lock 0，同线程 trylock %s（重复 lock 会死锁，非持有者 unlock 未定义，跳过）\n",
           err_name(normal_try));

    pthread_mutexattr_t attr;
    CHECK(pthread_mutexattr_init(&attr));

    pthread_mutex_t errorcheck;
    CHECK(pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_ERRORCHECK));
    CHECK(pthread_mutex_init(&errorcheck, &attr));
    CHECK(pthread_mutex_lock(&errorcheck));
    const int ec_relock = pthread_mutex_lock(&errorcheck);
    CHECK(pthread_mutex_unlock(&errorcheck));
    const int ec_other = unlock_by_other(&errorcheck);
    const int ec_double = pthread_mutex_unlock(&errorcheck);
    printf("ERRORCHECK：lock 0，重复 lock %s，非持有者 unlock %s，重复 unlock %s\n",
           err_name(ec_relock), err_name(ec_other), err_name(ec_double));
    CHECK(pthread_mutex_destroy(&errorcheck));

    pthread_mutex_t recursive;
    CHECK(pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE));
    CHECK(pthread_mutex_init(&recursive, &attr));
    CHECK(pthread_mutex_lock(&recursive));
    const int rec_relock = pthread_mutex_lock(&recursive);
    const int rec_other = unlock_by_other(&recursive);
    CHECK(pthread_mutex_unlock(&recursive));
    CHECK(pthread_mutex_unlock(&recursive));
    const int rec_after = pthread_mutex_trylock(&recursive);
    CHECK(pthread_mutex_unlock(&recursive));
    printf("RECURSIVE：lock 0，重复 lock %s，非持有者 unlock %s，解锁两次后 trylock %s\n",
           err_name(rec_relock), err_name(rec_other), err_name(rec_after));
    CHECK(pthread_mutex_destroy(&recursive));

    pthread_mutexattr_destroy(&attr);
    CHECK(pthread_mutex_destroy(&normal));
    return 0;
}
