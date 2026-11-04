// mode: run
// expect: 入队 100000，出队 100000，校验和 4999950000
// pthread 版：链表节点用 malloc 分配，wait_pop 在 while 里检查 !empty || closed。
#define _POSIX_C_SOURCE 200809L
#include <pthread.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct node {
    int value;
    struct node *next;
} node;

typedef struct {
    pthread_mutex_t mutex;
    pthread_cond_t cond;
    node *head;
    node *tail;
    int closed;
} queue;

static void die(const char *what, int rc) {
    fprintf(stderr, "%s: %s\n", what, strerror(rc));
    exit(1);
}

static void queue_init(queue *q) {
    int rc = pthread_mutex_init(&q->mutex, NULL);
    if (rc != 0) die("pthread_mutex_init", rc);
    rc = pthread_cond_init(&q->cond, NULL);
    if (rc != 0) die("pthread_cond_init", rc);
    q->head = q->tail = NULL;
    q->closed = 0;
}

static void queue_destroy(queue *q) {
    while (q->head != NULL) {
        node *n = q->head;
        q->head = n->next;
        free(n);
    }
    int rc = pthread_cond_destroy(&q->cond);
    if (rc != 0) die("pthread_cond_destroy", rc);
    rc = pthread_mutex_destroy(&q->mutex);
    if (rc != 0) die("pthread_mutex_destroy", rc);
}

static int queue_push(queue *q, int value) {
    node *n = malloc(sizeof *n);
    if (n == NULL) {
        fprintf(stderr, "malloc failed\n");
        exit(1);
    }
    n->value = value;
    n->next = NULL;

    int rc = pthread_mutex_lock(&q->mutex);
    if (rc != 0) die("pthread_mutex_lock", rc);
    if (q->closed) {
        rc = pthread_mutex_unlock(&q->mutex);
        if (rc != 0) die("pthread_mutex_unlock", rc);
        free(n);
        return 0;
    }
    if (q->tail == NULL) {
        q->head = q->tail = n;
    } else {
        q->tail->next = n;
        q->tail = n;
    }
    rc = pthread_mutex_unlock(&q->mutex);
    if (rc != 0) die("pthread_mutex_unlock", rc);
    rc = pthread_cond_signal(&q->cond);
    if (rc != 0) die("pthread_cond_signal", rc);
    return 1;
}
// ANCHOR_END: wait_pop

// ANCHOR: wait_pop
static int queue_wait_pop(queue *q, int *out) {
    int rc = pthread_mutex_lock(&q->mutex);
    if (rc != 0) die("pthread_mutex_lock", rc);
    while (q->head == NULL && !q->closed) {
        rc = pthread_cond_wait(&q->cond, &q->mutex);
        if (rc != 0) die("pthread_cond_wait", rc);
    }
    if (q->head == NULL) {
        rc = pthread_mutex_unlock(&q->mutex);
        if (rc != 0) die("pthread_mutex_unlock", rc);
        return 0;
    }
    node *n = q->head;
    q->head = n->next;
    if (q->head == NULL) {
        q->tail = NULL;
    }
    *out = n->value;
    rc = pthread_mutex_unlock(&q->mutex);
    if (rc != 0) die("pthread_mutex_unlock", rc);
    free(n);
    return 1;
}

// ANCHOR: close
static void queue_close(queue *q) {
    int rc = pthread_mutex_lock(&q->mutex);
    if (rc != 0) die("pthread_mutex_lock", rc);
    q->closed = 1;
    rc = pthread_mutex_unlock(&q->mutex);
    if (rc != 0) die("pthread_mutex_unlock", rc);
    rc = pthread_cond_broadcast(&q->cond);
    if (rc != 0) die("pthread_cond_broadcast", rc);
}
// ANCHOR_END: close

enum { kProducers = 2, kConsumers = 2, kTasks = 100000 };

static queue q;
static long pushed;
static long popped;
static unsigned long long checksum;
static pthread_mutex_t stats = PTHREAD_MUTEX_INITIALIZER;

static void *producer(void *arg) {
    int id = *(int *)arg;
    for (int i = id; i < kTasks; i += kProducers) {
        if (queue_push(&q, i)) {
            int rc = pthread_mutex_lock(&stats);
            if (rc != 0) die("pthread_mutex_lock", rc);
            ++pushed;
            rc = pthread_mutex_unlock(&stats);
            if (rc != 0) die("pthread_mutex_unlock", rc);
        }
    }
    return NULL;
}

static void *consumer(void *arg) {
    (void)arg;
    long n = 0;
    unsigned long long sum = 0;
    int value;
    while (queue_wait_pop(&q, &value)) {
        sum += (unsigned)value;
        ++n;
    }
    int rc = pthread_mutex_lock(&stats);
    if (rc != 0) die("pthread_mutex_lock", rc);
    popped += n;
    checksum += sum;
    rc = pthread_mutex_unlock(&stats);
    if (rc != 0) die("pthread_mutex_unlock", rc);
    return NULL;
}

int main(void) {
    queue_init(&q);
    pthread_t producers[kProducers];
    pthread_t consumers[kConsumers];
    int ids[kProducers];

    for (int i = 0; i < kProducers; ++i) {
        ids[i] = i;
        int rc = pthread_create(&producers[i], NULL, producer, &ids[i]);
        if (rc != 0) die("pthread_create", rc);
    }
    for (int i = 0; i < kConsumers; ++i) {
        int rc = pthread_create(&consumers[i], NULL, consumer, NULL);
        if (rc != 0) die("pthread_create", rc);
    }
    for (int i = 0; i < kProducers; ++i) {
        int rc = pthread_join(producers[i], NULL);
        if (rc != 0) die("pthread_join", rc);
    }
    queue_close(&q);
    for (int i = 0; i < kConsumers; ++i) {
        int rc = pthread_join(consumers[i], NULL);
        if (rc != 0) die("pthread_join", rc);
    }
    printf("入队 %ld，出队 %ld，校验和 %llu\n", pushed, popped, checksum);
    queue_destroy(&q);
    int rc = pthread_mutex_destroy(&stats);
    if (rc != 0) die("pthread_mutex_destroy", rc);
    return 0;
}
