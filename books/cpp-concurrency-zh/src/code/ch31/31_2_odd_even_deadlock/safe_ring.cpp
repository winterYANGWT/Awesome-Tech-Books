// 修法：偶数 rank 先发后收，奇数 rank 先收后发，环就不会闭合
#include <mpi.h>

#include <cstdio>

int main(int argc, char** argv) {
    MPI_Init(&argc, &argv);
    int rank = 0;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    int partner = rank ^ 1;
    int send = 100 + rank;
    int recv = -1;

    if (rank % 2 == 0) {
        MPI_Send(&send, 1, MPI_INT, partner, 0, MPI_COMM_WORLD);
        MPI_Recv(&recv, 1, MPI_INT, partner, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
    } else {
        MPI_Recv(&recv, 1, MPI_INT, partner, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        MPI_Send(&send, 1, MPI_INT, partner, 0, MPI_COMM_WORLD);
    }

    if (rank == 0)
        std::printf("safe：每对 rank 都完成了交换，rank 0 收到 %d\n", recv);
    MPI_Finalize();
    return 0;
}
