// mode: run
// expect: pthread_join 的退出值 == PTHREAD_CANCELED: 是
// expect: 清理函数执行次数: 1
// expect: 第二个线程拿到了互斥量: 是

#define _POSIX_C_SOURCE 200809L

#include <pthread.h>
#include <sched.h>
#include <stdatomic.h>
#include <stdio.h>

static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t cond = PTHREAD_COND_INITIALIZER;
static atomic_int ready = 0;
static atomic_int go = 0;
static atomic_int cleanups = 0;
static atomic_int probed = 0;

static void unlock_mutex(void *arg) {
  atomic_fetch_add(&cleanups, 1);
  pthread_mutex_unlock((pthread_mutex_t *)arg);
}

static void *worker(void *arg) {
  (void)arg;
  pthread_mutex_lock(&mutex);
  pthread_cleanup_push(unlock_mutex, &mutex);
  atomic_store(&ready, 1);
  for (;;) pthread_cond_wait(&cond, &mutex); /* 取消点；虚假唤醒后继续等 */
  pthread_cleanup_pop(0);
  pthread_mutex_unlock(&mutex);
  return NULL;
}

static void *prober(void *arg) {
  (void)arg;
  while (!atomic_load(&go)) {
    sched_yield();
  }
  pthread_mutex_lock(&mutex); /* 清理函数已经把锁解开 */
  atomic_store(&probed, 1);
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
  rc = pthread_cancel(victim);
  if (rc != 0) {
    fprintf(stderr, "pthread_cancel: %d\n", rc);
    return 1;
  }

  void *result = NULL;
  rc = pthread_join(victim, &result);
  if (rc != 0) {
    fprintf(stderr, "pthread_join victim: %d\n", rc);
    return 1;
  }
  printf("pthread_join 的退出值 == PTHREAD_CANCELED: %s\n",
         result == PTHREAD_CANCELED ? "是" : "否");
  printf("清理函数执行次数: %d\n", atomic_load(&cleanups));

  atomic_store(&go, 1);
  rc = pthread_join(second, NULL);
  if (rc != 0) {
    fprintf(stderr, "pthread_join prober: %d\n", rc);
    return 1;
  }
  printf("第二个线程拿到了互斥量: %s\n", atomic_load(&probed) ? "是" : "否");
  return 0;
}
