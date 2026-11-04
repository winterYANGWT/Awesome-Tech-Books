// mode: run
// flags: -fopenmp
// tsan: no（libgomp 未插桩）
// expect: sum=500000500000 expected=500000500000
#include <stdio.h>
#include <omp.h>

int main(void) {
    const int n = 1000000;
    long long sum = 0;

#pragma omp parallel num_threads(4)
    {
#pragma omp single
        {
#pragma omp taskloop reduction(+:sum) grainsize(1000)
            for (int i = 1; i <= n; ++i) {
                sum += i;
            }
        }
    }

    long long expected = (long long)n * (n + 1) / 2;
    printf("sum=%lld expected=%lld\n", sum, expected);
    return 0;
}
