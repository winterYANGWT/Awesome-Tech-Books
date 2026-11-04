// mode: run
// env: OMP_CANCELLATION=true OMP_NUM_THREADS=4
// flags: -fopenmp
// tsan: no（libgomp 未插桩）
// expect: found=1
// expect: cancellation=1

#include <omp.h>
#include <stdio.h>

int main(void) {
  int found = 0;

#pragma omp parallel
  {
#pragma omp for
    for (int i = 0; i < 1000000; ++i) {
      int seen = 0;
#pragma omp atomic read
      seen = found;
      if (i == 500000) {
#pragma omp atomic write
        found = 1;
#pragma omp cancel for
      }
      if (seen) {
        /* 规范规定的取消点：其他线程在这里看到取消已激活 */
#pragma omp cancellation point for
      }
    }
  }

  int final_found = 0;
#pragma omp atomic read
  final_found = found;
  printf("found=%d\n", final_found);
  printf("cancellation=%d\n", omp_get_cancellation());
  return 0;
}
