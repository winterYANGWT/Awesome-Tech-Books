// n=16、p=4：Scatter 每 rank 4 个元素，各自乘 2，再 Gather 回 root。
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

    const int n = 16;
    if (n % size != 0) {
        if (rank == 0) {
            std::fprintf(stderr, "n=%d 不能被 p=%d 整除\n", n, size);
        }
        MPI_Finalize();
        return 1;
    }
    const int local_n = n / size;

    std::vector<int> original;
    std::vector<int> result;
    if (rank == 0) {
        original.resize(static_cast<std::size_t>(n));
        result.resize(static_cast<std::size_t>(n));
        for (int i = 0; i < n; ++i) {
            original[static_cast<std::size_t>(i)] = i;
        }
    }

    std::vector<int> local(static_cast<std::size_t>(local_n), 0);
    MPI_Scatter(original.data(), local_n, MPI_INT, local.data(), local_n,
                MPI_INT, 0, MPI_COMM_WORLD);

    for (int& v : local) {
        v *= 2;
    }

    MPI_Gather(local.data(), local_n, MPI_INT, result.data(), local_n,
               MPI_INT, 0, MPI_COMM_WORLD);

    if (rank == 0) {
        std::printf("原向量:");
        for (int v : original) {
            std::printf(" %d", v);
        }
        std::printf("\n乘 2 后:");
        for (int v : result) {
            std::printf(" %d", v);
        }
        std::printf("\n");
    }

    MPI_Finalize();
    return 0;
}
// ANCHOR_END: core
