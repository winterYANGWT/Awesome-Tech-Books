// 反例：所有 rank 先 MPI_Ssend 再 MPI_Recv，形成环上等待
#include <mpi.h>

#include <cstdio>

int main(int argc, char** argv) {
    MPI_Init(&argc, &argv);
    int rank = 0, p = 0;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &p);

    int partner = rank ^ 1;
    int send = 100 + rank;
    int recv = -1;

    if (rank == 0) {
        std::printf("所有 rank 先 Ssend 再 Recv：配对的 rank 互相等待\n");
        std::fflush(stdout);
    }

    // Ssend 要等对方投递匹配的接收；所有 rank 都停在这里
    MPI_Ssend(&send, 1, MPI_INT, partner, 0, MPI_COMM_WORLD);
    MPI_Recv(&recv, 1, MPI_INT, partner, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

    if (rank == 0) std::printf("不应该到达这里：recv=%d\n", recv);
    MPI_Finalize();
    return 0;
}
