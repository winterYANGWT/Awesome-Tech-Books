// mode: run
// output: varies
// expect: dynamic_pthreads: best=60
#define _POSIX_C_SOURCE 200809L
#include <pthread.h>
#include <stdatomic.h>
#include <stdbool.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define N 12
#define MAXN 16
#define INF (INT_MAX / 2)
#define THREADS 4
#define CUTOFF 9
#define REPS 20

static int dist[N][N];

typedef struct {
    int city[MAXN];
    int n;
    int cost;
} Tour;

typedef struct {
    Tour *v;
    size_t n, cap;
} Stack;

static atomic_int best_cost = INF;
static pthread_mutex_t best_mutex = PTHREAD_MUTEX_INITIALIZER;
static Tour best_tour;

static pthread_mutex_t term_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t term_cv = PTHREAD_COND_INITIALIZER;
static Stack handoff;
static int idle, active, finished;
static atomic_int waiting_hint;
static atomic_int split_count;

static long long nodes[THREADS];

#define CHECK(call)                                                       \
    do {                                                                  \
        int rc_ = (call);                                                 \
        if (rc_ != 0) {                                                   \
            fprintf(stderr, "%s failed: %d\n", #call, rc_);               \
            exit(1);                                                      \
        }                                                                 \
    } while (0)

static void build_graph(void) {
    for (int i = 0; i < N; ++i)
        for (int j = 0; j < N; ++j)
            dist[i][j] = (i == j) ? 0 : ((i * 37 + j * 91 + 13) % 23) + 1;
}

static void stack_init(Stack *s, size_t cap) {
    s->v = malloc(cap * sizeof(Tour));
    if (!s->v) {
        fprintf(stderr, "out of memory\n");
        exit(1);
    }
    s->n = 0;
    s->cap = cap;
}

static void stack_push(Stack *s, Tour t) {
    if (s->n == s->cap) {
        s->cap *= 2;
        s->v = realloc(s->v, s->cap * sizeof(Tour));
        if (!s->v) {
            fprintf(stderr, "out of memory\n");
            exit(1);
        }
    }
    s->v[s->n++] = t;
}

static Tour stack_pop(Stack *s) {
    return s->v[--s->n];
}

static void stack_free(Stack *s) {
    free(s->v);
    s->v = NULL;
    s->n = s->cap = 0;
}

static int cmp_depth(const void *a, const void *b) {
    const Tour *x = a, *y = b;
    return (x->n > y->n) - (x->n < y->n);
}

static Stack partition_tree(int want) {
    Stack level, next;
    stack_init(&level, 16);
    Tour root = {{0}, 1, 0};
    stack_push(&level, root);
    while ((int)level.n < want) {
        stack_init(&next, level.n * 8 + 16);
        for (size_t i = 0; i < level.n; ++i) {
            Tour t = level.v[i];
            bool used[N] = {false};
            for (int j = 0; j < t.n; ++j) used[t.city[j]] = true;
            int last = t.city[t.n - 1];
            for (int v = 0; v < N; ++v) {
                if (used[v]) continue;
                Tour child = t;
                child.city[child.n++] = v;
                child.cost = t.cost + dist[last][v];
                stack_push(&next, child);
            }
        }
        stack_free(&level);
        level = next;
        if (level.n == 0) break;
    }
    return level;
}

static void try_update(int c, Tour t) {
    if (c >= atomic_load_explicit(&best_cost, memory_order_relaxed)) return;
    pthread_mutex_lock(&best_mutex);
    if (c < atomic_load_explicit(&best_cost, memory_order_relaxed)) {
        best_tour = t;
        best_tour.cost = c;
        atomic_store_explicit(&best_cost, c, memory_order_release);
    }
    pthread_mutex_unlock(&best_mutex);
}

/* 调用者持有 term_mutex。 */
static int split_locked(Stack *s) {
    qsort(s->v, s->n, sizeof(Tour), cmp_depth);
    size_t eligible = 0;
    while (eligible < s->n && s->v[eligible].n < CUTOFF) ++eligible;
    if (eligible < 2) return 0;

    Stack keep, give;
    stack_init(&keep, s->n);
    stack_init(&give, eligible / 2 + 1);
    for (size_t i = 0; i < s->n; ++i) {
        if (i < eligible && (i % 2) == 1) stack_push(&give, s->v[i]);
        else stack_push(&keep, s->v[i]);
    }
    stack_free(s);
    *s = keep;
    stack_free(&handoff);
    handoff = give;
    atomic_fetch_add(&split_count, 1);
    pthread_cond_signal(&term_cv);
    return 1;
}

static int try_split(Stack *s) {
    if (s->n < 2 || atomic_load_explicit(&waiting_hint, memory_order_relaxed) == 0)
        return 0;
    pthread_mutex_lock(&term_mutex);
    if (handoff.n != 0 ||
        atomic_load_explicit(&waiting_hint, memory_order_relaxed) == 0 ||
        s->n < 2) {
        pthread_mutex_unlock(&term_mutex);
        return 0;
    }
    int r = split_locked(s);
    pthread_mutex_unlock(&term_mutex);
    return r;
}

typedef struct {
    int id;
    Stack seed;
} Worker;

static void *search_worker(void *arg) {
    Worker *w = arg;
    int id = w->id;
    Stack *s = &w->seed;
    long long local = 0;

    for (;;) {
        if (s->n == 0) {
            pthread_mutex_lock(&term_mutex);
            ++idle;
            atomic_store_explicit(&waiting_hint, idle, memory_order_relaxed);
            while (handoff.n == 0 && !finished) {
                if (idle == active) {
                    finished = 1;
                    pthread_cond_broadcast(&term_cv);
                    break;
                }
                pthread_cond_wait(&term_cv, &term_mutex);
            }
            if (handoff.n) {
                stack_free(s);
                *s = handoff;
                handoff.v = NULL;
                handoff.n = handoff.cap = 0;
            }
            --idle;
            atomic_store_explicit(&waiting_hint, idle, memory_order_relaxed);
            if (finished && s->n == 0) {
                --active;
                pthread_cond_broadcast(&term_cv);
                pthread_mutex_unlock(&term_mutex);
                nodes[id] = local;
                return NULL;
            }
            pthread_mutex_unlock(&term_mutex);
        }
        if (s->n == 0) continue;

        while (s->n) {
            if (s->n >= 2 && try_split(s)) break;
            Tour t = stack_pop(s);
            if (t.cost >= atomic_load_explicit(&best_cost, memory_order_relaxed))
                continue;
            if (t.n == N) {
                ++local;
                try_update(t.cost + dist[t.city[t.n - 1]][0], t);
                continue;
            }
            ++local;
            bool used[N] = {false};
            for (int i = 0; i < t.n; ++i) used[t.city[i]] = true;
            int last = t.city[t.n - 1];
            for (int v = N - 1; v >= 0; --v) {
                if (used[v]) continue;
                int c = t.cost + dist[last][v];
                if (c >= atomic_load_explicit(&best_cost, memory_order_relaxed))
                    continue;
                Tour child = t;
                child.city[child.n++] = v;
                child.cost = c;
                stack_push(s, child);
            }
        }
    }
}

static double now_ms(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec * 1000.0 + ts.tv_nsec / 1e6;
}

int main(void) {
    build_graph();
    Stack parts = partition_tree(THREADS);
    printf("dynamic_pthreads: cities=%d threads=%d cutoff=%d reps=%d\n",
           N, THREADS, CUTOFF, REPS);

    double total_ms = 0;
    long long total_nodes[THREADS] = {0};
    int total_splits = 0;
    int final_best = INF;

    for (int rep = 0; rep < REPS; ++rep) {
        atomic_store(&best_cost, INF);
        atomic_store(&split_count, 0);
        atomic_store(&waiting_hint, 0);
        idle = 0;
        active = THREADS;
        finished = 0;
        stack_free(&handoff);

        Worker workers[THREADS];
        pthread_t tids[THREADS];
        for (int id = 0; id < THREADS; ++id) {
            workers[id].id = id;
            if (id == 0) {
                stack_init(&workers[id].seed, parts.n);
                for (size_t i = 0; i < parts.n; ++i)
                    stack_push(&workers[id].seed, parts.v[i]);
            } else {
                stack_init(&workers[id].seed, 1024);
            }
        }

        double t0 = now_ms();
        for (int id = 0; id < THREADS; ++id)
            CHECK(pthread_create(&tids[id], NULL, search_worker, &workers[id]));
        for (int id = 0; id < THREADS; ++id)
            CHECK(pthread_join(tids[id], NULL));
        double t1 = now_ms();

        total_ms += t1 - t0;
        for (int id = 0; id < THREADS; ++id) {
            total_nodes[id] += nodes[id];
            stack_free(&workers[id].seed);
        }
        total_splits += atomic_load(&split_count);
        final_best = atomic_load(&best_cost);
    }

    printf("dynamic_pthreads: best=%d splits=%d nodes=", final_best, total_splits);
    long long sum = 0;
    for (int id = 0; id < THREADS; ++id) {
        printf("%lld%s", total_nodes[id], id + 1 == THREADS ? "" : ",");
        sum += total_nodes[id];
    }
    printf(" total=%lld ms_per_rep=%.3f\n", sum, total_ms / REPS);
    stack_free(&parts);
    return 0;
}
