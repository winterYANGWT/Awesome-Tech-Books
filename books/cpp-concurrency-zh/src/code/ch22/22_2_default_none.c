// mode: fail
// flags: -fopenmp
// expect: n.*not specified|not specified.*n
#include <stdio.h>

int main(void) {
    int n = 100;
    long sum = 0;

    /* 故意漏写 shared(n)：default(none) 要求每个外部变量都列出来。 */
#pragma omp parallel for default(none) reduction(+:sum)
    for (int i = 0; i < n; ++i) {
        sum += i;
    }
    printf("%ld\n", sum);
    return 0;
}
