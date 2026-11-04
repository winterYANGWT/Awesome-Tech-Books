// 环上先 Irecv 再 Isend，两个请求都提交后用 Waitall 一起完成。
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

    const int left = (rank - 1 + size) % size;
    const int right = (rank + 1) % size;
    const int send_value = rank * 10;
    int recv_value = -1;

    MPI_Request req[2];
    MPI_Irecv(&recv_value, 1, MPI_INT, left, 0, MPI_COMM_WORLD, &req[0]);
    MPI_Isend(&send_value, 1, MPI_INT, right, 0, MPI_COMM_WORLD, &req[1]);
    MPI_Waitall(2, req, MPI_STATUSES_IGNORE);

    std::vector<int> all(static_cast<std::size_t>(size), 0);
    MPI_Gather(&recv_value, 1, MPI_INT, all.data(), 1, MPI_INT, 0,
               MPI_COMM_WORLD);

    if (rank == 0) {
        for (int i = 0; i < size; ++i) {
            std::printf("rank %d 收到 %d\n", i, all[static_cast<std::size_t>(i)]);
        }
    }

    MPI_Finalize();
    return 0;
}
// ANCHOR_END: core
