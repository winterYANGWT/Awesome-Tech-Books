// mode: run
// flags: -fopenmp
// output: varies
// tsan: no（libgomp 未插桩）
// expect: threads=8
#include <stdio.h>
#include <omp.h>

static double f(double x) { return 4.0 / (1.0 + x * x); }

/* 分块算局部和，再逐块进入未命名 critical 加到共享 pi。
   块取小一点，是为了让临界区争用真的出现在计时里。 */
static double pi_critical(long n, int threads, double *ms) {
    const long chunk = 64;
    const double h = 1.0 / (double)n;
    double pi = 0.0;
    double best = 1e30;
    for (int rep = 0; rep < 3; ++rep) {
        double t0 = omp_get_wtime();
#pragma omp parallel for num_threads(threads) schedule(static)
        for (long base = 0; base < n; base += chunk) {
            long end = base + chunk;
            if (end > n) end = n;
            double local = 0.0;
            for (long i = base; i < end; ++i) {
                double x = ((double)i + 0.5) * h;
                local += f(x);
            }
#pragma omp critical
            pi += local;
        }
        double dt = (omp_get_wtime() - t0) * 1000.0;
        if (rep == 0 || dt < best) best = dt;
        pi = 0.0;
    }
    *ms = best;
    /* 再算一次，拿到与计时那次对应的值（数值只差末几位）。 */
    double sum = 0.0;
#pragma omp parallel for num_threads(threads) schedule(static)
    for (long base = 0; base < n; base += chunk) {
        long end = base + chunk;
        if (end > n) end = n;
        double local = 0.0;
        for (long i = base; i < end; ++i) {
            double x = ((double)i + 0.5) * h;
            local += f(x);
        }
#pragma omp critical
        sum += local;
    }
    return sum * h;
}

int main(void) {
    const long n = 40000000;
    const int counts[] = {1, 2, 4, 8};
    for (unsigned k = 0; k < sizeof counts / sizeof counts[0]; ++k) {
        double ms = 0.0;
        double pi = pi_critical(n, counts[k], &ms);
        printf("threads=%d pi=%.15f ms=%.3f\n", counts[k], pi, ms);
    }
    return 0;
}
