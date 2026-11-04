// mode: run
// expect: join 后 pthread_kill\(t, 0\)=0
#define _POSIX_C_SOURCE 200809L
#include <pthread.h>
#include <signal.h>
#include <stdio.h>

static void *done(void *arg)
{
    (void)arg;
    return NULL;
}

int main(void)
{
    pthread_t t;
    if (pthread_create(&t, NULL, done, NULL) != 0)
        return 1;
    if (pthread_join(t, NULL) != 0)
        return 1;
    int rc = pthread_kill(t, 0);
    printf("join 后 pthread_kill(t, 0)=%d\n", rc);
    printf("这个 0 只说明 glibc 没有认出旧句柄，不能说明线程还活着\n");
    return 0;
}
