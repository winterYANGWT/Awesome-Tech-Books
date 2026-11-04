// mode: fail
// flags: -fopenmp
// expect: nowait.*not (valid|allowed)
#include <stdio.h>

int main(void) {
    int a[16];
    /* nowait 不能出现在合并的 parallel for 上。 */
#pragma omp parallel for nowait
    for (int i = 0; i < 16; ++i) {
        a[i] = i;
    }
    printf("%d\n", a[0]);
    return 0;
}
