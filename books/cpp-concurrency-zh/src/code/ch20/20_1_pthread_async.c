// mode: hang
// expect: 仍未结束

#define _POSIX_C_SOURCE 200809L

#include <pthread.h>
#include <sched.h>
#include <stdatomic.h>
#include <stdio.h>

static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
static atomic_int ready = 0;

static void *worker(void *arg) {
  (void)arg;
  pthread_mutex_lock(&mutex); /* 先拿锁：异步取消下再获取互斥量不可移植 */
  int rc = pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, NULL);
  if (rc != 0) {
    fprintf(stderr, "pthread_setcanceltype: %d\n", rc);
    pthread_mutex_unlock(&mutex);
    return NULL;
  }
  atomic_store(&ready, 1);
  /* 持有互斥量时忙等，异步取消会在这里打断线程 */
  volatile unsigned long spin = 0;
  for (;;) {
    spin = spin + 1;
  }
  return NULL; /* 不会到达 */
}

int main(void) {
  pthread_t thread;
  int rc = pthread_create(&thread, NULL, worker, NULL);
  if (rc != 0) {
    fprintf(stderr, "pthread_create: %d\n", rc);
    return 1;
  }

  while (!atomic_load(&ready)) {
    sched_yield();
  }
  printf("主线程：工作线程已持有互斥量，请求异步取消\n");
  rc = pthread_cancel(thread);
  if (rc != 0) {
    fprintf(stderr, "pthread_cancel: %d\n", rc);
    return 1;
  }

  printf("主线程：尝试锁同一把互斥量\n");
  pthread_mutex_lock(&mutex); /* 线程已死，锁没有释放 */
  printf("主线程：拿到了互斥量（不会发生）\n");
  pthread_mutex_unlock(&mutex);
  pthread_join(thread, NULL);
  return 0;
}
