// mode: mpi
#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>

int main(int argc, char **argv) {
    MPI_Init(&argc, &argv);

    int rank = -1;
    int size = 0;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    double a = 0.0;
    double b = 0.0;
    int n = 0;

    if (rank == 0) {
        if (scanf("%lf %lf %d", &a, &b, &n) != 3) {
            fprintf(stderr, "用法：stdin 上给 a b n\n");
            MPI_Abort(MPI_COMM_WORLD, 1);
        }
    }

    // 三个值各广播一次，所有 rank 都拿到同一份输入。
    MPI_Bcast(&a, 1, MPI_DOUBLE, 0, MPI_COMM_WORLD);
    MPI_Bcast(&b, 1, MPI_DOUBLE, 0, MPI_COMM_WORLD);
    MPI_Bcast(&n, 1, MPI_INT, 0, MPI_COMM_WORLD);

    const double local = (a + b) * (rank + 1);
    if (rank != 0) {
        double message[3] = {a, b, local};
        MPI_Send(message, 3, MPI_DOUBLE, 0, 2, MPI_COMM_WORLD);
    } else {
        double total = local;
        printf("rank 0：读到 a=%.1f b=%.1f n=%d，本地值 %.1f\n", a, b, n,
               local);
        for (int src = 1; src < size; ++src) {
            double message[3] = {0.0, 0.0, 0.0};
            MPI_Recv(message, 3, MPI_DOUBLE, src, 2, MPI_COMM_WORLD,
                     MPI_STATUS_IGNORE);
            printf("rank %d：拿到 a=%.1f b=%.1f，本地值 %.1f\n", src,
                   message[0], message[1], message[2]);
            total += message[2];
        }
        printf("汇总：%.1f\n", total);
    }

    MPI_Finalize();
    return 0;
}
