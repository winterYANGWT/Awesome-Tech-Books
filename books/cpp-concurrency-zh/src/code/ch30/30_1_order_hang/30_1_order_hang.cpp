// 反例：rank 0 调 Bcast(root=1)，其余 rank 调 Reduce(root=0)。
// 集合操作按顺序和 root 匹配；rank 0 在 Bcast 里等 rank 1，其余 rank 的 Reduce 先返回后卡在 Finalize。
#include <mpi.h>

#include <cstdio>

// ANCHOR: core
int main(int argc, char** argv) {
    MPI_Init(&argc, &argv);

    int rank = 0;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    int value = rank + 1;
    if (rank == 0) {
        std::fprintf(stderr, "rank 0: 进入 MPI_Bcast(root=1)\n");
        std::fflush(stderr);
        MPI_Bcast(&value, 1, MPI_INT, 1, MPI_COMM_WORLD);
    } else {
        std::fprintf(stderr, "rank %d: 进入 MPI_Reduce\n", rank);
        std::fflush(stderr);
        int sum = 0;
        MPI_Reduce(&value, &sum, 1, MPI_INT, MPI_SUM, 0, MPI_COMM_WORLD);
    }

    std::fprintf(stderr, "rank %d: 集合操作返回\n", rank);
    MPI_Finalize();
    return 0;
}
// ANCHOR_END: core
