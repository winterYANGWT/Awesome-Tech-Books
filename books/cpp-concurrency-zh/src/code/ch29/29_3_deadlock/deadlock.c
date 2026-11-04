// mode: mpi
#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

enum { BYTES = 1024 * 1024 };

int main(int argc, char **argv) {
    MPI_Init(&argc, &argv);
    setvbuf(stdout, NULL, _IONBF, 0);

    int rank = -1;
    int size = 0;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    const int right = (rank + 1) % size;
    const int left = (rank + size - 1) % size;

    char *buffer = (char *)malloc(BYTES);
    if (buffer == NULL) {
        MPI_Abort(MPI_COMM_WORLD, 1);
    }
    memset(buffer, 'a' + rank, BYTES);

    printf("rank %d：开始 Send %d 字节给 %d\n", rank, BYTES, right);
    MPI_Send(buffer, BYTES, MPI_CHAR, right, 0, MPI_COMM_WORLD);
    printf("rank %d：Send 返回，开始 Recv 来自 %d\n", rank, left);
    MPI_Recv(buffer, BYTES, MPI_CHAR, left, 0, MPI_COMM_WORLD,
             MPI_STATUS_IGNORE);
    printf("rank %d：收到，首字节是 '%c'\n", rank, buffer[0]);

    free(buffer);
    MPI_Finalize();
    return 0;
}
