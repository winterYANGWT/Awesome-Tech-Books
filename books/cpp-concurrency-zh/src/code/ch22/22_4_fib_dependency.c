// mode: run
// flags: -fopenmp
// output: varies
// tsan: no（libgomp 未插桩）
// expect: runs=200 bad=[1-9][0-9]*
#include <stdio.h>

#define N 20

int main(void) {
    const int runs = 200;
    int bad = 0;

    for (int r = 0; r < runs; ++r) {
        long long f[N];
        for (int i = 0; i < N; ++i) f[i] = -1;
        f[0] = 1;
        f[1] = 1;

        /* f[i] 依赖前两项：编译通过，但跨迭代的写和读没有同步。 */
#pragma omp parallel for num_threads(4) schedule(static)
        for (int i = 2; i < N; ++i) {
            f[i] = f[i - 1] + f[i - 2];
        }

        int wrong = 0;
        for (int i = 0; i < N; ++i) {
            if (f[i] <= 0) wrong = 1;
        }
        if (wrong) ++bad;
    }
    printf("runs=%d bad=%d\n", runs, bad);
    return 0;
}
