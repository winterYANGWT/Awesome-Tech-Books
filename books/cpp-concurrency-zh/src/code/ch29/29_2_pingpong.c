// mode: mpi
// np: 2
// output: varies
// expect: 往返 1000 次，校验和 1000
#include <mpi.h>
#include <stdio.h>

enum { ROUNDS = 1000 };

int main(int argc, char **argv) {
    MPI_Init(&argc, &argv);

    int rank = -1;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    if (rank == 0) {
        double token = 1.0;
        double sum = 0.0;
        const double start = MPI_Wtime();
        for (int i = 0; i < ROUNDS; ++i) {
            MPI_Send(&token, 1, MPI_DOUBLE, 1, 7, MPI_COMM_WORLD);
            MPI_Recv(&token, 1, MPI_DOUBLE, 1, 7, MPI_COMM_WORLD,
                     MPI_STATUS_IGNORE);
            sum += token;
        }
        const double elapsed = MPI_Wtime() - start;
        printf("往返 %d 次，校验和 %.1f\n", ROUNDS, sum);
        printf("平均每次往返 %.2f 微秒\n", elapsed / ROUNDS * 1e6);
    } else if (rank == 1) {
        double token = 0.0;
        for (int i = 0; i < ROUNDS; ++i) {
            MPI_Recv(&token, 1, MPI_DOUBLE, 0, 7, MPI_COMM_WORLD,
                     MPI_STATUS_IGNORE);
            MPI_Send(&token, 1, MPI_DOUBLE, 0, 7, MPI_COMM_WORLD);
        }
    }

    MPI_Finalize();
    return 0;
}
