// mode: mpi
#include <mpi.h>
#include <stdio.h>
#include <string.h>

int main(int argc, char **argv) {
    MPI_Init(&argc, &argv);
    setvbuf(stdout, NULL, _IONBF, 0);

    int safe = 0;
    for (int i = 1; i < argc; ++i) {
        if (strcmp(argv[i], "--safe") == 0) {
            safe = 1;
        }
    }

    int rank = -1;
    int size = 0;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    const int right = (rank + 1) % size;
    const int left = (rank + size - 1) % size;
    int send_value = rank * 10;
    int recv_value = -1;

    if (!safe) {
        // 每个 rank 都先 Ssend：没有 rank 先 post 接收，全部停在发送。
        printf("rank %d：Ssend %d 给 %d\n", rank, send_value, right);
        MPI_Ssend(&send_value, 1, MPI_INT, right, 0, MPI_COMM_WORLD);
        MPI_Recv(&recv_value, 1, MPI_INT, left, 0, MPI_COMM_WORLD,
                 MPI_STATUS_IGNORE);
        printf("rank %d：收到 %d\n", rank, recv_value);
    } else {
        // 偶数先发后收，奇数先收后发，环上的等待方向就一致了。
        if (rank % 2 == 0) {
            MPI_Ssend(&send_value, 1, MPI_INT, right, 0, MPI_COMM_WORLD);
            MPI_Recv(&recv_value, 1, MPI_INT, left, 0, MPI_COMM_WORLD,
                     MPI_STATUS_IGNORE);
        } else {
            MPI_Recv(&recv_value, 1, MPI_INT, left, 0, MPI_COMM_WORLD,
                     MPI_STATUS_IGNORE);
            MPI_Ssend(&send_value, 1, MPI_INT, right, 0, MPI_COMM_WORLD);
        }
        if (rank == 0) {
            printf("safe：%d 个 rank 完成环交换，rank 0 收到 %d\n", size,
                   recv_value);
        }
    }

    MPI_Finalize();
    return 0;
}
