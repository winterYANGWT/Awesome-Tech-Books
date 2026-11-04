// n=10、p=4、counts={3,3,2,2}：Scatterv 分发不等长块，Gatherv 拼回完整向量。
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

    if (size != 4) {
        if (rank == 0) {
            std::fprintf(stderr, "本示例按 p=4 写 counts={3,3,2,2}\n");
        }
        MPI_Finalize();
        return 1;
    }

    const int counts[4] = {3, 3, 2, 2};
    const int displs[4] = {0, 3, 6, 8};  // 单位是 int 的 extent，不是字节
    const int n = 10;

    std::vector<int> original;
    if (rank == 0) {
        original.resize(static_cast<std::size_t>(n));
        for (int i = 0; i < n; ++i) {
            original[static_cast<std::size_t>(i)] = i;
        }
    }

    std::vector<int> local(static_cast<std::size_t>(counts[rank]), 0);
    MPI_Scatterv(original.data(), counts, displs, MPI_INT,
                 local.data(), counts[rank], MPI_INT, 0, MPI_COMM_WORLD);

    int count_out[4] = {};
    MPI_Gather(&counts[rank], 1, MPI_INT, count_out, 1, MPI_INT, 0,
               MPI_COMM_WORLD);

    std::vector<int> joined;
    if (rank == 0) {
        joined.resize(static_cast<std::size_t>(n));
    }
    MPI_Gatherv(local.data(), counts[rank], MPI_INT, joined.data(), counts,
                displs, MPI_INT, 0, MPI_COMM_WORLD);

    if (rank == 0) {
        std::printf("各 rank 收到:");
        for (int i = 0; i < size; ++i) {
            std::printf(" %d", count_out[i]);
        }
        std::printf("\n拼接后:");
        for (int v : joined) {
            std::printf(" %d", v);
        }
        std::printf("\n");
    }

    MPI_Finalize();
    return 0;
}
// ANCHOR_END: core
