// root 的输入直接放在 recvbuf 里，sendbuf 传 MPI_IN_PLACE；非 root 仍然正常发送。
#include <mpi.h>

#include <cstdio>

// ANCHOR: core
int main(int argc, char** argv) {
    MPI_Init(&argc, &argv);

    int rank = 0;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    const int mine = rank + 1;
    int normal = 0;
    MPI_Reduce(&mine, &normal, 1, MPI_INT, MPI_SUM, 0, MPI_COMM_WORLD);

    // 普通做法：root 的输入在 sendbuf，输出在 recvbuf。
    int in_place = mine;  // root 把自己的值先放进接收缓冲区
    MPI_Reduce(rank == 0 ? MPI_IN_PLACE : &mine, &in_place, 1, MPI_INT,
               MPI_SUM, 0, MPI_COMM_WORLD);

    if (rank == 0) {
        std::printf("普通 Reduce: %d\n", normal);
        std::printf("IN_PLACE Reduce: %d\n", in_place);
        std::printf("两种写法一致: %s\n", normal == in_place ? "true" : "false");
    }

    MPI_Finalize();
    return 0;
}
// ANCHOR_END: core
