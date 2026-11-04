// mode: mpi
#include <mpi.h>
#include <stdio.h>
#include <string.h>

int main(int argc, char **argv) {
    MPI_Init(&argc, &argv);

    int rank = -1;
    int size = 0;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    if (rank != 0) {
        // 每个进程把问候语发给 0 号，0 号按来源顺序打印。
        char greeting[64];
        snprintf(greeting, sizeof greeting, "rank %d/%d 报到", rank, size);
        MPI_Send(greeting, (int)strlen(greeting) + 1, MPI_CHAR, 0, 100,
                 MPI_COMM_WORLD);
    } else {
        printf("MPI 世界：%d 个进程\n", size);
        for (int src = 1; src < size; ++src) {
            char greeting[64];
            MPI_Status status;
            MPI_Recv(greeting, (int)sizeof greeting, MPI_CHAR, src, 100,
                     MPI_COMM_WORLD, &status);
            printf("%s（来源 %d，tag %d）\n", greeting, status.MPI_SOURCE,
                   status.MPI_TAG);
        }
    }

    MPI_Finalize();
    return 0;
}
