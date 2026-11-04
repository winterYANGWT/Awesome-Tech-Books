// mode: run
// flags: -fopenmp
// env: OMP_SCHEDULE=dynamic,2
// output: varies
// tsan: no（libgomp 未插桩）
// expect: after t0: 0 4 8 12
#include <stdio.h>
#include <omp.h>

#define N 16
#define T 4

static void show(const char *tag) {
    int owner[N];
    for (int i = 0; i < N; ++i) owner[i] = -1;
#pragma omp parallel for num_threads(T) schedule(runtime)
    for (int i = 0; i < N; ++i) {
        /* 给每次迭代一点工作量，否则一个线程可能在别人启动前领完所有块。 */
        volatile unsigned long burn = 0;
        for (unsigned long j = 0; j < 2000000UL; ++j) burn += j;
        owner[i] = omp_get_thread_num();
    }
    printf("%s", tag);
    for (int id = 0; id < T; ++id) {
        printf(" t%d:", id);
        for (int i = 0; i < N; ++i) {
            if (owner[i] == id) printf(" %d", i);
        }
    }
    printf("\n");
}

int main(void) {
    omp_sched_t kind = omp_sched_static;
    int chunk = 0;
    omp_get_schedule(&kind, &chunk);
    printf("env kind=%d chunk=%d\n", (int)kind, chunk);
    show("before");

    omp_set_schedule(omp_sched_static, 1);
    show("after");
    return 0;
}
