// mode: run
// flags: -fopenmp
// output: varies
// tsan: no（libgomp 未插桩）
// expect: sent=400 received=400 left=0 0 0 0
#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <stdatomic.h>
#include <omp.h>

#define T 4
#define CAP 1024
#define SEND 100

typedef struct {
    int items[CAP];
    int head;
    int tail;
    atomic_int enqueued;
    atomic_int dequeued;
} Queue;

static void enqueue(Queue *q, int msg) {
    /* 入队只动队尾，但两个线程可能同时入同一队列。 */
#pragma omp critical(enqueue)
    {
        q->items[q->tail % CAP] = msg;
        q->tail++;
        atomic_fetch_add(&q->enqueued, 1);
    }
}

static int dequeue(Queue *q, int *out) {
    int enq = atomic_load(&q->enqueued);
    int deq = atomic_load(&q->dequeued);
    if (enq == deq) return 0;

    if (enq - deq == 1) {
        /* 只剩一个元素时，队头和队尾可能重合，必须和入队互斥。 */
        int got = 0;
#pragma omp critical(enqueue)
        {
            enq = atomic_load(&q->enqueued);
            deq = atomic_load(&q->dequeued);
            if (enq != deq) {
                *out = q->items[q->head % CAP];
                q->head++;
                atomic_fetch_add(&q->dequeued, 1);
                got = 1;
            }
        }
        return got;
    }

    /* 队列里还有多个元素：只有拥有者出队，队头和队尾不重合。 */
    *out = q->items[q->head % CAP];
    q->head++;
    atomic_fetch_add(&q->dequeued, 1);
    return 1;
}

int main(void) {
    Queue q[T];
    for (int i = 0; i < T; ++i) {
        q[i].head = q[i].tail = 0;
        atomic_init(&q[i].enqueued, 0);
        atomic_init(&q[i].dequeued, 0);
    }

    atomic_int done_sending;
    atomic_init(&done_sending, 0);
    int sent[T] = {0};
    int received[T] = {0};

#pragma omp parallel num_threads(T)
    {
        int id = omp_get_thread_num();
        unsigned seed = 12345u + (unsigned)id * 7919u;

        for (int m = 0; m < SEND; ++m) {
            int target = (int)(rand_r(&seed) % T);
            enqueue(&q[target], id * 1000 + m);
            sent[id]++;
        }
        atomic_fetch_add(&done_sending, 1);

        /* 没有条件变量：收不到就轮询，直到所有人发完且自己的队列为空。 */
        for (;;) {
            int msg = 0;
            if (dequeue(&q[id], &msg)) {
                received[id]++;
            } else if (atomic_load(&done_sending) == T) {
                int enq = atomic_load(&q[id].enqueued);
                int deq = atomic_load(&q[id].dequeued);
                if (enq == deq) break;
            }
        }
    }

    int total_sent = 0, total_received = 0;
    printf("sent=");
    for (int i = 0; i < T; ++i) total_sent += sent[i];
    printf("%d received=", total_sent);
    for (int i = 0; i < T; ++i) total_received += received[i];
    printf("%d left=", total_received);
    for (int i = 0; i < T; ++i) {
        int left = atomic_load(&q[i].enqueued) - atomic_load(&q[i].dequeued);
        printf("%s%d", i ? " " : "", left);
    }
    printf("\n");
    return 0;
}
