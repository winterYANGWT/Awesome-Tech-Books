// 能量守恒的时序：rank 2 先空闲，rank 1 的工作还在途中，收齐 3 单位才终止
#include <mpi.h>

#include <chrono>
#include <cstdio>
#include <thread>

int main(int argc, char** argv) {
    MPI_Init(&argc, &argv);
    int rank = 0;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    constexpr int WORK = 30, ENERGY = 31;
    if (rank == 2) {
        double e = 1.0;
        MPI_Send(&e, 1, MPI_DOUBLE, 0, ENERGY, MPI_COMM_WORLD);
    } else if (rank == 1) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        double work[2] = {1.0, 0.5};  // 1 条工作，带 0.5 单位能量
        MPI_Send(work, 2, MPI_DOUBLE, 0, WORK, MPI_COMM_WORLD);
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        double e = 0.5;
        MPI_Send(&e, 1, MPI_DOUBLE, 0, ENERGY, MPI_COMM_WORLD);
    } else {
        double own = 1.0;    // rank 0 自己的 1 单位
        double total = 0.0;  // 已完成、并入 rank 0 账上的能量
        std::printf("rank 0: 自己的能量 1.000\n");

        double e = 0.0;
        MPI_Recv(&e, 1, MPI_DOUBLE, 2, ENERGY, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        total += e;
        std::printf("rank 2 -> rank 0: 1.000（空闲），总计 %.3f\n", total);

        double work[2] = {0.0, 0.0};
        MPI_Recv(work, 2, MPI_DOUBLE, 1, WORK, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        double pending = work[1];  // 工作还没执行，能量先记在本地
        std::printf("rank 1 -> rank 0: 工作到达，带 %.3f（暂存，等执行）\n", pending);

        const double merged = own + pending;
        std::this_thread::sleep_for(std::chrono::milliseconds(50));  // 模拟执行
        total += merged;  // 本地工作全部完成，才并入总数
        own = 0.0;
        pending = 0.0;
        std::printf("rank 0: 执行完本地任务，并入 %.3f，总计 %.3f\n", merged, total);

        MPI_Recv(&e, 1, MPI_DOUBLE, 1, ENERGY, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        total += e;
        std::printf("rank 1 -> rank 0: 完成后交回 %.3f，总计 %.3f\n", e, total);
        std::printf("收齐 %.3f，广播终止\n", total);
    }
    MPI_Finalize();
    return 0;
}
