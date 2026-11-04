// mode: mpi
// expect: rank 0：发出 0，收到 30
#include <mpi.h>
#include <stdio.h>

int main(int argc, char **argv) {
    MPI_Init(&argc, &argv);

    int rank = -1;
    int size = 0;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    const int right = (rank + 1) % size;
    const int left = (rank + size - 1) % size;
    const int send_value = rank * 10;
    int recv_value = -1;

    // 一次调用同时发和收，MPI 负责让环上的两边都前进。
    MPI_Sendrecv(&send_value, 1, MPI_INT, right, 0, &recv_value, 1, MPI_INT,
                 left, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

    if (rank != 0) {
        MPI_Send(&recv_value, 1, MPI_INT, 0, 1, MPI_COMM_WORLD);
    } else {
        printf("rank 0：发出 %d，收到 %d\n", send_value, recv_value);
        for (int src = 1; src < size; ++src) {
            int value = -1;
            MPI_Recv(&value, 1, MPI_INT, src, 1, MPI_COMM_WORLD,
                     MPI_STATUS_IGNORE);
            printf("rank %d：收到 %d\n", src, value);
        }
    }

    MPI_Finalize();
    return 0;
}
