// mode: hang
// expect: 仍未结束

#define _POSIX_C_SOURCE 200809L

#include <pthread.h>
#include <sched.h>
#include <stdatomic.h>
#include <stdio.h>

static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t cond = PTHREAD_COND_INITIALIZER;
static atomic_int ready = 0;
static atomic_int go = 0;

static void *worker(void *arg) {
  (void)arg;
  pthread_mutex_lock(&mutex);
  atomic_store(&ready, 1);
  for (;;) pthread_cond_wait(&cond, &mutex); /* 取消点；没有清理函数 */
  pthread_mutex_unlock(&mutex);
  return NULL;
}

static void *prober(void *arg) {
  (void)arg;
  while (!atomic_load(&go)) {
    sched_yield();
  }
  printf("第二个线程：尝试锁同一把互斥量\n");
  pthread_mutex_lock(&mutex); /* 锁仍被已取消的线程持有 */
  printf("第二个线程：拿到了互斥量（不会发生）\n");
  pthread_mutex_unlock(&mutex);
  return NULL;
}

int main(void) {
  pthread_t victim;
  pthread_t second;
  int rc = pthread_create(&victim, NULL, worker, NULL);
  if (rc != 0) {
    fprintf(stderr, "pthread_create victim: %d\n", rc);
    return 1;
  }
  rc = pthread_create(&second, NULL, prober, NULL);
  if (rc != 0) {
    fprintf(stderr, "pthread_create prober: %d\n", rc);
    return 1;
  }

  while (!atomic_load(&ready)) {
    sched_yield();
  }
  pthread_cancel(victim);
  pthread_join(victim, NULL);
  printf("主线程：工作线程已被取消并 join\n");

  atomic_store(&go, 1);
  printf("主线程：让第二个线程去锁同一把互斥量\n");
  pthread_join(second, NULL);
  return 0;
}
