// mode: run
// flags: -fopenmp
// output: varies
// tsan: no（libgomp 未插桩）
// expect: ^1,[0-9]
#include <stdio.h>
#include <stdlib.h>
#include <omp.h>

static double f(double x) { return 4.0 / (1.0 + x * x); }

static double pi_reduction(long n, int threads, double *ms) {
    const long chunk = 4096;
    const double h = 1.0 / (double)n;
    double pi = 0.0;
    double best = 1e30;
    for (int rep = 0; rep < 3; ++rep) {
        double t0 = omp_get_wtime();
        double sum = 0.0;
#pragma omp parallel for num_threads(threads) schedule(static) reduction(+:sum)
        for (long base = 0; base < n; base += chunk) {
            long end = base + chunk;
            if (end > n) end = n;
            for (long i = base; i < end; ++i) {
                double x = ((double)i + 0.5) * h;
                sum += f(x);
            }
        }
        double dt = (omp_get_wtime() - t0) * 1000.0;
        if (rep == 0 || dt < best) best = dt;
        pi = sum * h;
    }
    *ms = best;
    return pi;
}

int main(void) {
    const long n = 200000000;
    const int counts[] = {1, 2, 4, 8};
    printf("threads,ms\n");
    for (unsigned k = 0; k < sizeof counts / sizeof counts[0]; ++k) {
        double ms = 0.0;
        double pi = pi_reduction(n, counts[k], &ms);
        printf("%d,%.3f\n", counts[k], ms);
        /* stdout 只留 CSV；需要核对 π 时设 PI_CHECK=1，值走 stderr。 */
        if (getenv("PI_CHECK") != NULL) {
            fprintf(stderr, "threads=%d pi=%.17g\n", counts[k], pi);
        }
    }
    return 0;
}
