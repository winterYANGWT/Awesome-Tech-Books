// mode: run
// output: varies
// expect: impl,read_99_9,read_80_20
// tsan: no（计时示例，TSan 下锁开销会盖过被测差异）
#define _POSIX_C_SOURCE 200809L

#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#define INITIAL_KEYS 512
#define KEY_RANGE 2048
#define THREADS 4
#define OPS 60000

enum impl_kind { IMPL_WHOLE, IMPL_PER_NODE, IMPL_RWLOCK };

typedef struct node {
    int key;
    pthread_mutex_t mutex;
    struct node *next;
} node;

typedef struct {
    node *head;
    pthread_mutex_t whole;
    pthread_rwlock_t rwlock;
    enum impl_kind kind;
} list;

typedef struct {
    list *l;
    pthread_barrier_t *start;
    unsigned seed;
    int read_cut;
    int write_cut;
} worker_arg;

static node *node_new(int key) {
    node *n = (node *)calloc(1, sizeof(node));
    if (!n) {
        abort();
    }
    n->key = key;
    if (pthread_mutex_init(&n->mutex, NULL) != 0) {
        abort();
    }
    return n;
}

static void node_free(node *n) {
    pthread_mutex_destroy(&n->mutex);
    free(n);
}

static void list_init(list *l, enum impl_kind kind) {
    l->kind = kind;
    l->head = node_new(0);
    if (pthread_mutex_init(&l->whole, NULL) != 0) {
        abort();
    }
    if (pthread_rwlock_init(&l->rwlock, NULL) != 0) {
        abort();
    }
}

static void list_destroy(list *l) {
    node *p = l->head;
    while (p) {
        node *next = p->next;
        node_free(p);
        p = next;
    }
    pthread_mutex_destroy(&l->whole);
    pthread_rwlock_destroy(&l->rwlock);
}

static int member_locked(list *l, int key) {
    node *p = l->head->next;
    while (p && p->key < key) {
        p = p->next;
    }
    return p && p->key == key;
}

static int insert_locked(list *l, int key) {
    node *prev = l->head;
    node *cur = prev->next;
    while (cur && cur->key < key) {
        prev = cur;
        cur = cur->next;
    }
    if (cur && cur->key == key) {
        return 0;
    }
    node *n = node_new(key);
    n->next = cur;
    prev->next = n;
    return 1;
}

static int delete_locked(list *l, int key) {
    node *prev = l->head;
    node *cur = prev->next;
    while (cur && cur->key < key) {
        prev = cur;
        cur = cur->next;
    }
    if (!cur || cur->key != key) {
        return 0;
    }
    prev->next = cur->next;
    node_free(cur);
    return 1;
}

// ANCHOR: member
static int per_node_member(list *l, int key) {
    pthread_mutex_lock(&l->head->mutex);
    node *p = l->head->next;
    if (p) {
        pthread_mutex_lock(&p->mutex);
    }
    pthread_mutex_unlock(&l->head->mutex);
    while (p) {
        if (p->key == key) {
            pthread_mutex_unlock(&p->mutex);
            return 1;
        }
        node *next = p->next;
        if (next) {
            pthread_mutex_lock(&next->mutex);
        }
        pthread_mutex_unlock(&p->mutex);
        p = next;
    }
    return 0;
}
// ANCHOR_END: member

static int per_node_insert(list *l, int key) {
    pthread_mutex_lock(&l->head->mutex);
    node *prev = l->head;
    node *cur = prev->next;
    if (cur) {
        pthread_mutex_lock(&cur->mutex);
    }
    while (cur && cur->key < key) {
        node *next = cur->next;
        if (next) {
            pthread_mutex_lock(&next->mutex);
        }
        pthread_mutex_unlock(&prev->mutex);
        prev = cur;
        cur = next;
    }
    if (cur && cur->key == key) {
        pthread_mutex_unlock(&cur->mutex);
        pthread_mutex_unlock(&prev->mutex);
        return 0;
    }
    node *n = node_new(key);
    n->next = cur;
    prev->next = n;
    pthread_mutex_unlock(&prev->mutex);
    if (cur) {
        pthread_mutex_unlock(&cur->mutex);
    }
    return 1;
}

static int per_node_delete(list *l, int key) {
    pthread_mutex_lock(&l->head->mutex);
    node *prev = l->head;
    node *cur = prev->next;
    if (cur) {
        pthread_mutex_lock(&cur->mutex);
    }
    while (cur && cur->key < key) {
        node *next = cur->next;
        if (next) {
            pthread_mutex_lock(&next->mutex);
        }
        pthread_mutex_unlock(&prev->mutex);
        prev = cur;
        cur = next;
    }
    if (!cur || cur->key != key) {
        if (cur) {
            pthread_mutex_unlock(&cur->mutex);
        }
        pthread_mutex_unlock(&prev->mutex);
        return 0;
    }
    prev->next = cur->next;
    pthread_mutex_unlock(&cur->mutex);
    pthread_mutex_unlock(&prev->mutex);
    node_free(cur);
    return 1;
}

static int list_member(list *l, int key) {
    switch (l->kind) {
    case IMPL_WHOLE:
        pthread_mutex_lock(&l->whole);
        {
            int r = member_locked(l, key);
            pthread_mutex_unlock(&l->whole);
            return r;
        }
    case IMPL_RWLOCK:
        pthread_rwlock_rdlock(&l->rwlock);
        {
            int r = member_locked(l, key);
            pthread_rwlock_unlock(&l->rwlock);
            return r;
        }
    default:
        return per_node_member(l, key);
    }
}

static int list_insert(list *l, int key) {
    switch (l->kind) {
    case IMPL_WHOLE:
        pthread_mutex_lock(&l->whole);
        {
            int r = insert_locked(l, key);
            pthread_mutex_unlock(&l->whole);
            return r;
        }
    case IMPL_RWLOCK:
        pthread_rwlock_wrlock(&l->rwlock);
        {
            int r = insert_locked(l, key);
            pthread_rwlock_unlock(&l->rwlock);
            return r;
        }
    default:
        return per_node_insert(l, key);
    }
}

static int list_delete(list *l, int key) {
    switch (l->kind) {
    case IMPL_WHOLE:
        pthread_mutex_lock(&l->whole);
        {
            int r = delete_locked(l, key);
            pthread_mutex_unlock(&l->whole);
            return r;
        }
    case IMPL_RWLOCK:
        pthread_rwlock_wrlock(&l->rwlock);
        {
            int r = delete_locked(l, key);
            pthread_rwlock_unlock(&l->rwlock);
            return r;
        }
    default:
        return per_node_delete(l, key);
    }
}

static unsigned xorshift(unsigned *state) {
    unsigned x = *state;
    x ^= x << 13;
    x ^= x >> 17;
    x ^= x << 5;
    *state = x;
    return x;
}

static void *worker(void *arg) {
    worker_arg *wa = (worker_arg *)arg;
    unsigned state = wa->seed;
    const int per_thread = OPS / THREADS;
    pthread_barrier_wait(wa->start);
    for (int i = 0; i < per_thread; ++i) {
        const unsigned r = xorshift(&state) % 10000u;
        const int key = (int)(xorshift(&state) % KEY_RANGE);
        if (r < (unsigned)wa->read_cut) {
            (void)list_member(wa->l, key);
        } else if (r < (unsigned)(wa->read_cut + wa->write_cut / 2)) {
            (void)list_insert(wa->l, key);
        } else {
            (void)list_delete(wa->l, key);
        }
    }
    return NULL;
}

static int list_is_sorted(list *l) {
    int last = -1;
    for (node *p = l->head->next; p; p = p->next) {
        if (p->key <= last) {
            return 0;
        }
        last = p->key;
    }
    return 1;
}

static double run_one(enum impl_kind kind, int read_cut, int write_cut) {
    list l;
    list_init(&l, kind);
    for (int key = 0; key < INITIAL_KEYS; ++key) {
        (void)list_insert(&l, key);
    }

    pthread_barrier_t start;
    pthread_barrier_init(&start, NULL, THREADS + 1);
    pthread_t threads[THREADS];
    worker_arg args[THREADS];
    for (int t = 0; t < THREADS; ++t) {
        args[t].l = &l;
        args[t].start = &start;
        args[t].seed = 0x9e3779b9u + (unsigned)t * 0x85ebca6bu;
        args[t].read_cut = read_cut;
        args[t].write_cut = write_cut;
        pthread_create(&threads[t], NULL, worker, &args[t]);
    }

    struct timespec begin;
    struct timespec end;
    clock_gettime(CLOCK_MONOTONIC, &begin);
    pthread_barrier_wait(&start);
    for (int t = 0; t < THREADS; ++t) {
        pthread_join(threads[t], NULL);
    }
    clock_gettime(CLOCK_MONOTONIC, &end);

    if (!list_is_sorted(&l)) {
        fprintf(stderr, "list order broken\n");
        exit(1);
    }
    list_destroy(&l);
    pthread_barrier_destroy(&start);
    return (double)(end.tv_sec - begin.tv_sec) * 1000.0 +
           (double)(end.tv_nsec - begin.tv_nsec) / 1e6;
}

int main(void) {
    const char *names[3] = {"whole_mutex", "per_node", "rwlock"};
    const enum impl_kind kinds[3] = {IMPL_WHOLE, IMPL_PER_NODE, IMPL_RWLOCK};
    double t99[3];
    double t80[3];
    for (int m = 0; m < 3; ++m) {
        t99[m] = run_one(kinds[m], 9990, 10);
        t80[m] = run_one(kinds[m], 8000, 2000);
    }

    printf("impl,read_99_9,read_80_20\n");
    for (int m = 0; m < 3; ++m) {
        printf("%s,%.1f,%.1f\n", names[m], t99[m], t80[m]);
    }
    return 0;
}
