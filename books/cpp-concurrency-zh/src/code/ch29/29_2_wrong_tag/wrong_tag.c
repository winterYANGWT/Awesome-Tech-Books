// mode: mpi
// np: 2
#include <mpi.h>
#include <stdio.h>

int main(int argc, char **argv) {
    MPI_Init(&argc, &argv);
    setvbuf(stdout, NULL, _IONBF, 0);

    int rank = -1;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    if (rank == 0) {
        printf("rank 0：给 rank 1 发 tag=1\n");
    } else {
        printf("rank 1：等 rank 0 的 tag=2\n");
    }
    MPI_Barrier(MPI_COMM_WORLD);

    double value = 42.0;
    MPI_Status status;
    if (rank == 0) {
        MPI_Send(&value, 1, MPI_DOUBLE, 1, 1, MPI_COMM_WORLD);
        printf("rank 0：Send 返回，改等 rank 1 的 tag=2\n");
        MPI_Recv(&value, 1, MPI_DOUBLE, 1, 2, MPI_COMM_WORLD, &status);
        printf("rank 0：收到 %.1f\n", value);
    } else {
        MPI_Recv(&value, 1, MPI_DOUBLE, 0, 2, MPI_COMM_WORLD, &status);
        printf("rank 1：收到 %.1f\n", value);
        MPI_Send(&value, 1, MPI_DOUBLE, 0, 2, MPI_COMM_WORLD);
    }

    MPI_Finalize();
    return 0;
}
