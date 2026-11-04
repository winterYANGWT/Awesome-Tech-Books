// mode: run
// flags: -fopenmp
// env: OMP_DYNAMIC=false
// tsan: no（libgomp 未插桩）
// expect: team=4 ids=0 1 2 3
#include <stdio.h>

#ifdef _OPENMP
#include <omp.h>
#endif

int main(void) {
    int seen[8] = {0};
    int team = 1;

#ifdef _OPENMP
    /* 每个线程只写自己的槽位；块尾的隐式屏障保证主线程看到全部写入。 */
#pragma omp parallel num_threads(4)
    {
        int id = omp_get_thread_num();
        int n = omp_get_num_threads();
        seen[id] = 1;
#pragma omp single
        team = n; /* single 只有一个线程写，末尾也有隐式屏障 */
    }
#else
    /* 没有 -fopenmp 时 pragma 被忽略，这里走单线程分支。 */
    seen[0] = 1;
#endif

    printf("team=%d ids=", team);
    for (int i = 0; i < team; ++i) {
        printf("%s%d", i == 0 ? "" : " ", seen[i] ? i : -1);
    }
    printf("\n");
    return 0;
}
