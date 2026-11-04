// 每个 rank 贡献 rank+1：Reduce 只让 root 拿到和，Allreduce 让所有 rank 都拿到。
#include <mpi.h>

#include <cstdio>
#include <vector>

// ANCHOR: core
int main(int argc, char** argv) {
    MPI_Init(&argc, &argv);

    int rank = 0;
    int size = 0;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    const int mine = rank + 1;
    int reduced = 0;
    MPI_Reduce(&mine, &reduced, 1, MPI_INT, MPI_SUM, 0, MPI_COMM_WORLD);

    int all = 0;
    MPI_Allreduce(&mine, &all, 1, MPI_INT, MPI_SUM, MPI_COMM_WORLD);

    if (rank == 0) {
        std::printf("MPI_Reduce 到 root: %d\n", reduced);
    }

    // 各 rank 的 Allreduce 结果按 rank 顺序收回，输出顺序就固定了。
    std::vector<int> values(static_cast<std::size_t>(size), 0);
    MPI_Gather(&all, 1, MPI_INT, values.data(), 1, MPI_INT, 0, MPI_COMM_WORLD);

    if (rank == 0) {
        std::printf("Allreduce 各 rank:");
        for (int i = 0; i < size; ++i) {
            std::printf(" %d", values[static_cast<std::size_t>(i)]);
        }
        std::printf("\n");
    }

    MPI_Finalize();
    return 0;
}
// ANCHOR_END: core
