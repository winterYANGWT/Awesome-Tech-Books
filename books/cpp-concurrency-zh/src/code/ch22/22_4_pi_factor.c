// mode: run
// flags: -fopenmp
// output: varies
// tsan: no（libgomp 未插桩）
// expect: threads=4
#include <stdio.h>
#include <omp.h>

/* Leibniz 级数：π/4 = 1 - 1/3 + 1/5 - …。
   factor 由 k 的奇偶直接算出，不再依赖上一轮；但 private 仍然需要。 */
static double leibniz(long n, int threads, int private_factor) {
    double sum = 0.0;
    double factor = 0.0;
    if (private_factor) {
#pragma omp parallel for num_threads(threads) schedule(static) reduction(+:sum) private(factor)
        for (long k = 0; k < n; ++k) {
            factor = (k % 2 == 0 ? 4.0 : -4.0) / (2.0 * (double)k + 1.0);
            sum += factor;
        }
    } else {
#pragma omp parallel for num_threads(threads) schedule(static) reduction(+:sum)
        for (long k = 0; k < n; ++k) {
            factor = (k % 2 == 0 ? 4.0 : -4.0) / (2.0 * (double)k + 1.0);
            sum += factor; /* factor 是共享的：别的线程会覆盖它 */
        }
    }
    return sum;
}

static double serial_leibniz(long n) {
    double sum = 0.0;
    for (long k = 0; k < n; ++k) {
        double factor = (k % 2 == 0 ? 4.0 : -4.0) / (2.0 * (double)k + 1.0);
        sum += factor;
    }
    return sum;
}

int main(void) {
    const long n = 50000000;
    double serial = serial_leibniz(n);

    printf("shared_factor(4 threads)=%.12f\n", leibniz(n, 4, 0));
    printf("private_factor:\n");
    const int counts[] = {1, 2, 4};
    for (unsigned k = 0; k < sizeof counts / sizeof counts[0]; ++k) {
        double pi = leibniz(n, counts[k], 1);
        printf("threads=%d pi=%.15f diff=%.3e\n",
               counts[k], pi, pi > serial ? pi - serial : serial - pi);
    }
    return 0;
}
