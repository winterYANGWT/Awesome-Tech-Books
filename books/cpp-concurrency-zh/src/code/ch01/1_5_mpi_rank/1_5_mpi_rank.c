// mode: mpi
// np: 4
// expect: PID 各不相同：true
#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

int main(int argc, char** argv) {
    // ANCHOR: core
    MPI_Init(&argc, &argv);

    int rank = -1;
    int size = 0;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    const long pid = (long)getpid();
    int* ranks = (int*)malloc((size_t)size * sizeof *ranks);
    long* pids = (long*)malloc((size_t)size * sizeof *pids);
    if (ranks == NULL || pids == NULL) {
        fprintf(stderr, "malloc failed\n");
        MPI_Abort(MPI_COMM_WORLD, 1);
    }

    // 两个数组都按 rank 顺序落到 rank 0，打印顺序就固定了。
    MPI_Gather(&rank, 1, MPI_INT, ranks, 1, MPI_INT, 0, MPI_COMM_WORLD);
    MPI_Gather(&pid, 1, MPI_LONG, pids, 1, MPI_LONG, 0, MPI_COMM_WORLD);

    if (rank == 0) {
        printf("MPI 进程数：%d\n", size);
        printf("rank 顺序：");
        for (int i = 0; i < size; ++i) {
            printf("%s%d", i == 0 ? "" : " ", ranks[i]);
        }
        printf("\n");

        int distinct = 1;
        for (int i = 0; i < size; ++i) {
            for (int j = i + 1; j < size; ++j) {
                if (pids[i] == pids[j]) distinct = 0;
            }
        }
        printf("PID 各不相同：%s\n", distinct ? "true" : "false");
    }

    free(ranks);
    free(pids);
    MPI_Finalize();
    // ANCHOR_END: core
    return 0;
}
