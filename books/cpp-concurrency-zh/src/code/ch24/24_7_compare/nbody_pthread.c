// mode: run
// output: varies
// expect: threads,ms
#define _POSIX_C_SOURCE 200809L
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <math.h>
#include <limits.h>

#define NB_N 800
#define NB_STEPS 200
#define NB_DT 0.001
#define NB_SOFTENING 1e-9
#define LINE 64

typedef struct {
    double x, y, z;
} Vec3;

typedef struct {
    Vec3 pos, vel;
    double mass;
} Particle;

typedef struct __attribute__((aligned(LINE))) {
    Vec3 f[NB_N];
} ForceRow;

typedef struct {
    int id, threads;
    Particle *p;
    Vec3 *forces;
    ForceRow *rows;
    pthread_barrier_t *bar;
} Worker;

static void init_particles(Particle *p) {
    unsigned long long s = 12345;
    for (int i = 0; i < NB_N; ++i) {
        double r[7];
        for (int j = 0; j < 7; ++j) {
            s = s * 6364136223846793005ULL + 1442695040888963407ULL;
            r[j] = (double)(s >> 11) / 9007199254740992.0;
        }
        p[i].pos = (Vec3){r[0] * 2 - 1, r[1] * 2 - 1, r[2] * 2 - 1};
        p[i].vel = (Vec3){r[3] * 0.1, r[4] * 0.1, r[5] * 0.1};
        p[i].mass = 0.5 + r[6];
    }
}

static double checksum(const Particle *p) {
    double sum = 0;
    for (int q = 0; q < NB_N; ++q)
        sum += p[q].mass * (p[q].pos.x + 2 * p[q].pos.y + 3 * p[q].pos.z);
    return sum;
}

static void *worker(void *arg) {
    Worker *w = arg;
    int q0 = (int)((long)NB_N * w->id / w->threads);
    int q1 = (int)((long)NB_N * (w->id + 1) / w->threads);
    ForceRow *row = &w->rows[w->id];

    for (int step = 0; step < NB_STEPS; ++step) {
        for (int q = 0; q < NB_N; ++q) row->f[q] = (Vec3){0, 0, 0};
        for (int q = q0; q < q1; ++q) {
            for (int k = q + 1; k < NB_N; ++k) {
                double dx = w->p[k].pos.x - w->p[q].pos.x;
                double dy = w->p[k].pos.y - w->p[q].pos.y;
                double dz = w->p[k].pos.z - w->p[q].pos.z;
                double r2 = dx * dx + dy * dy + dz * dz + NB_SOFTENING;
                double inv = 1.0 / (r2 * sqrt(r2));
                double fx = w->p[q].mass * w->p[k].mass * inv * dx;
                double fy = w->p[q].mass * w->p[k].mass * inv * dy;
                double fz = w->p[q].mass * w->p[k].mass * inv * dz;
                row->f[q].x += fx;
                row->f[q].y += fy;
                row->f[q].z += fz;
                row->f[k].x -= fx;
                row->f[k].y -= fy;
                row->f[k].z -= fz;
            }
        }
        pthread_barrier_wait(w->bar);

        for (int q = q0; q < q1; ++q) {
            Vec3 a = {0, 0, 0};
            for (int r = 0; r < w->threads; ++r) {
                a.x += w->rows[r].f[q].x;
                a.y += w->rows[r].f[q].y;
                a.z += w->rows[r].f[q].z;
            }
            w->forces[q] = a;
            w->p[q].vel.x += a.x / w->p[q].mass * NB_DT;
            w->p[q].vel.y += a.y / w->p[q].mass * NB_DT;
            w->p[q].vel.z += a.z / w->p[q].mass * NB_DT;
            w->p[q].pos.x += w->p[q].vel.x * NB_DT;
            w->p[q].pos.y += w->p[q].vel.y * NB_DT;
            w->p[q].pos.z += w->p[q].vel.z * NB_DT;
        }
        pthread_barrier_wait(w->bar);
    }
    return NULL;
}

static double now_ms(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec * 1000.0 + ts.tv_nsec / 1e6;
}

static double run_nbody(int threads, double *out_sum) {
    Particle *p = malloc(NB_N * sizeof(Particle));
    Vec3 *forces = malloc(NB_N * sizeof(Vec3));
    ForceRow *rows = aligned_alloc(LINE, (size_t)threads * sizeof(ForceRow));
    pthread_barrier_t bar;
    if (!p || !forces || !rows) {
        fprintf(stderr, "out of memory\n");
        exit(1);
    }
    init_particles(p);
    pthread_barrier_init(&bar, NULL, (unsigned)threads);

    Worker *args = malloc((size_t)threads * sizeof(Worker));
    pthread_t *tids = malloc((size_t)threads * sizeof(pthread_t));
    for (int id = 0; id < threads; ++id)
        args[id] = (Worker){id, threads, p, forces, rows, &bar};

    double t0 = now_ms();
    for (int id = 0; id < threads; ++id) {
        int rc = pthread_create(&tids[id], NULL, worker, &args[id]);
        if (rc != 0) {
            fprintf(stderr, "pthread_create failed: %d\n", rc);
            exit(1);
        }
    }
    for (int id = 0; id < threads; ++id) pthread_join(tids[id], NULL);
    double t1 = now_ms();

    *out_sum = checksum(p);
    pthread_barrier_destroy(&bar);
    free(p);
    free(forces);
    free(rows);
    free(args);
    free(tids);
    return t1 - t0;
}

#define TSP_N 12
#define TSP_MAXN 16
static int tsp_dist[TSP_N][TSP_N];
static int tsp_best;

static void tsp_dfs(int *city, int n, int cost, int used[TSP_N]) {
    if (n == TSP_N) {
        int c = cost + tsp_dist[city[n - 1]][0];
        if (c < tsp_best) tsp_best = c;
        return;
    }
    int last = city[n - 1];
    for (int v = 0; v < TSP_N; ++v) {
        if (used[v]) continue;
        int c = cost + tsp_dist[last][v];
        if (c >= tsp_best) continue;
        used[v] = 1;
        city[n] = v;
        tsp_dfs(city, n + 1, c, used);
        used[v] = 0;
    }
}

static int tsp_check(void) {
    for (int i = 0; i < TSP_N; ++i)
        for (int j = 0; j < TSP_N; ++j)
            tsp_dist[i][j] = (i == j) ? 0 : ((i * 37 + j * 91 + 13) % 23) + 1;
    tsp_best = INT_MAX / 2;
    int city[TSP_MAXN] = {0};
    int used[TSP_N] = {0};
    used[0] = 1;
    tsp_dfs(city, 1, 0, used);
    return tsp_best;
}

int main(void) {
    fprintf(stderr, "pthread tsp_best=%d\n", tsp_check());
    printf("threads,ms\n");
    for (int threads = 1; threads <= 16; threads *= 2) {
        double times[3];
        double sum = 0;
        for (int rep = 0; rep < 3; ++rep) times[rep] = run_nbody(threads, &sum);
        for (int a = 0; a < 3; ++a)
            for (int b = a + 1; b < 3; ++b)
                if (times[b] < times[a]) {
                    double tmp = times[a];
                    times[a] = times[b];
                    times[b] = tmp;
                }
        fprintf(stderr, "pthread threads=%d checksum=%.17g\n", threads, sum);
        printf("%d,%.3f\n", threads, times[1]);
    }
    return 0;
}
