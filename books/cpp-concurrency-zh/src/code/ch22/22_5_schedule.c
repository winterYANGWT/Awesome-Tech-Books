// mode: run
// flags: -fopenmp -lm
// output: varies
// tsan: no（libgomp 未插桩）
// expect: ^scheme,thread,iters,work,ms
#include <stdio.h>
#include <math.h>
#include <omp.h>

#define N 64
#define T 4

/* 工作量随下标增长：i 越大，sin 的次数越多。 */
static double work_of(int i) {
    double x = 0.0;
    int reps = i * 10000;
    for (int j = 0; j < reps; ++j) {
        x += fabs(sin((double)(i * 1000 + j) * 1e-4));
    }
    return x;
}

static void run_scheme(const char *name, omp_sched_t kind, int chunk) {
    int iters[T][N];
    int count[T] = {0};
    double work[T] = {0};
    double ms[T] = {0};

    omp_set_schedule(kind, chunk);
#pragma omp parallel num_threads(T)
    {
        int id = omp_get_thread_num();
        double first = 0.0, last = 0.0;
#pragma omp for schedule(runtime)
        for (int i = 0; i < N; ++i) {
            double now = omp_get_wtime();
            if (count[id] == 0) first = now;
            last = now;
            iters[id][count[id]++] = i;
            work[id] += work_of(i);
        }
        ms[id] = count[id] ? (last - first) * 1000.0 : 0.0;
    }

    for (int id = 0; id < T; ++id) {
        printf("%s,%d,\"", name, id);
        for (int k = 0; k < count[id]; ++k) {
            printf("%s%d", k ? " " : "", iters[id][k]);
        }
        printf("\",%.3f,%.3f\n", work[id], ms[id]);
    }
}

int main(void) {
    printf("scheme,thread,iters,work,ms\n");
    run_scheme("static", omp_sched_static, 0);
    run_scheme("static,1", omp_sched_static, 1);
    run_scheme("dynamic,1", omp_sched_dynamic, 1);
    run_scheme("guided", omp_sched_guided, 0);
    return 0;
}
