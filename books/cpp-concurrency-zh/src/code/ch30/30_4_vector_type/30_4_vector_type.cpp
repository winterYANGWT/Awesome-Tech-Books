// 4x4 矩阵的列在内存中不连续；用 MPI_Type_vector(4, 1, 4) 把一列描述成一个元素。
#include <mpi.h>

#include <cstdio>

// ANCHOR: core
int main(int argc, char** argv) {
    MPI_Init(&argc, &argv);

    int rank = 0;
    int size = 0;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    if (size < 2) {
        if (rank == 0) {
            std::fprintf(stderr, "本示例需要至少 2 个 rank\n");
        }
        MPI_Finalize();
        return 1;
    }

    double a[4][4];
    for (int i = 0; i < 4; ++i) {
        for (int j = 0; j < 4; ++j) {
            a[i][j] = static_cast<double>(i * 10 + j);
        }
    }

    MPI_Datatype col_type = MPI_DATATYPE_NULL;
    MPI_Type_vector(4, 1, 4, MPI_DOUBLE, &col_type);  // 4 块、每块 1 个、跨 4 个
    MPI_Type_commit(&col_type);

    if (rank == 0) {
        MPI_Send(&a[0][0], 1, col_type, 1, 0, MPI_COMM_WORLD);
        double back[4] = {};
        MPI_Recv(back, 4, MPI_DOUBLE, 1, 1, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

        std::printf("矩阵第 0 列（按行）:");
        for (int i = 0; i < 4; ++i) {
            std::printf(" %.0f", a[i][0]);
        }
        std::printf("\nrank 1 收到并送回:");
        for (double v : back) {
            std::printf(" %.0f", v);
        }
        std::printf("\n");
    } else if (rank == 1) {
        double col[4] = {};
        MPI_Recv(col, 4, MPI_DOUBLE, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        MPI_Send(col, 4, MPI_DOUBLE, 0, 1, MPI_COMM_WORLD);
    }

    MPI_Type_free(&col_type);
    MPI_Finalize();
    return 0;
}
// ANCHOR_END: core
