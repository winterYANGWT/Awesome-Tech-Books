// mode: fail
// flags: -fopenmp
// expect: ‘nowait’ is not valid for ‘#pragma omp parallel for’

#include <stdio.h>

int main(void) {
    /* 合并的 parallel for 不是 worksharing 构造，不能再加 nowait。 */
#pragma omp parallel for nowait
    for (int i = 0; i < 4; ++i) {
        printf("%d\n", i);
    }
    return 0;
}
