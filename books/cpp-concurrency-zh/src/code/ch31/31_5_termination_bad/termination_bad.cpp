// 反例：用“空闲 rank 计数器”做终止，工作还在途中就退出
#include <mpi.h>

#include <cassert>
#include <chrono>
#include <cstdio>
#include <thread>
#include <vector>

int main(int argc, char** argv) {
    MPI_Init(&argc, &argv);
    int rank = 0, p = 0;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &p);
    if (p != 3) {
        if (rank == 0) std::printf("这个反例需要 3 个 rank\n");
        MPI_Finalize();
        return 1;
    }

    constexpr int WORK = 20, IDLE = 21;
    if (rank == 1) {
        // 一个大消息让工作留在发送缓冲里；随后空闲通知先到 rank 0
        std::vector<int> work(1 << 18, 7);
        MPI_Request req;
        MPI_Isend(work.data(), static_cast<int>(work.size()), MPI_INT, 0, WORK,
                  MPI_COMM_WORLD, &req);
        MPI_Request_free(&req);
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        int one = 1;
        MPI_Send(&one, 1, MPI_INT, 0, IDLE, MPI_COMM_WORLD);
        std::this_thread::sleep_for(std::chrono::milliseconds(300));
    } else if (rank == 2) {
        int one = 1;
        MPI_Send(&one, 1, MPI_INT, 0, IDLE, MPI_COMM_WORLD);
    } else {
        std::printf("rank 0: 等空闲通知，工作消息还没有取\n");
        std::fflush(stdout);
        int one = 0;
        MPI_Recv(&one, 1, MPI_INT, 2, IDLE, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        MPI_Recv(&one, 1, MPI_INT, 1, IDLE, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        const int idle = 3;  // 自己 + rank 1 + rank 2
        std::fprintf(stderr, "rank 0: 空闲计数=%d，判定搜索结束\n", idle);
        int lost = 1;  // rank 1 的工作还在路上
        std::fprintf(stderr, "丢失任务数=%d\n", lost);
        std::fflush(stderr);
        assert(lost == 0 && "还有工作未处理");
    }
    MPI_Finalize();
    return 0;
}
