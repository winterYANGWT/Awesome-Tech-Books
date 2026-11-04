// mode: run
// output: varies
// expect: OpenMP 执行流
#include <omp.h>
#include <stdio.h>

int main(void) {
    // ANCHOR: core
#pragma omp parallel
    {
        printf("OpenMP 执行流 %d/%d\n", omp_get_thread_num(),
               omp_get_num_threads());
    }
    // ANCHOR_END: core
    return 0;
}
