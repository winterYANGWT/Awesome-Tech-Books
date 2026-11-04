// mode: run
// flags: -fopenmp
// tsan: no（libgomp 未插桩）
// expect: task=832040 serial=832040
#include <stdio.h>
#include <omp.h>

/* 小于阈值就串行，避免任务太碎；返回值和串行版本逐位相同。 */
static long long fib(int n) {
    if (n < 2) return n;
    if (n < 12) {
        long long a = 0, b = 1;
        for (int i = 2; i <= n; ++i) {
            long long c = a + b;
            a = b;
            b = c;
        }
        return b;
    }

    long long a = 0, b = 0;
#pragma omp task shared(a) firstprivate(n)
    a = fib(n - 1);
#pragma omp task shared(b) firstprivate(n)
    b = fib(n - 2);
#pragma omp taskwait
    return a + b;
}

static long long fib_serial(int n) {
    long long a = 0, b = 1;
    for (int i = 2; i <= n; ++i) {
        long long c = a + b;
        a = b;
        b = c;
    }
    return b;
}

int main(void) {
    const int n = 30;
    long long result = 0;
#pragma omp parallel num_threads(4)
    {
#pragma omp single
        result = fib(n);
    }
    printf("task=%lld serial=%lld\n", result, fib_serial(n));
    return 0;
}
