// 按行块划分 A 和 y；每个 rank 只存 x 的一段，每轮用 Allgather 拼出完整 x。
#include <mpi.h>

#include <cstdio>
#include <vector>

// ANCHOR: core
int main(int argc, char** argv) {
    MPI_Init(&argc, &argv);

    int rank = 0;
    int size = 0;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    const int n = 1024;
    if (n % size != 0) {
        if (rank == 0) {
            std::fprintf(stderr, "n=%d 不能被 p=%d 整除\n", n, size);
        }
        MPI_Finalize();
        return 1;
    }
    const int local_n = n / size;
    const int row0 = rank * local_n;

    // A 按行块存放：第 i 个本地行对应全局行 row0+i。
    std::vector<double> a(static_cast<std::size_t>(local_n) * n);
    for (int i = 0; i < local_n; ++i) {
        for (int j = 0; j < n; ++j) {
            a[static_cast<std::size_t>(i) * n + j] =
                static_cast<double>((row0 + i + j) % 7);
        }
    }

    std::vector<double> local_x(static_cast<std::size_t>(local_n), 1.0);
    std::vector<double> x(static_cast<std::size_t>(n), 0.0);
    std::vector<double> local_y(static_cast<std::size_t>(local_n), 0.0);

    const int iterations = 3;
    MPI_Barrier(MPI_COMM_WORLD);
    const double t0 = MPI_Wtime();

    for (int it = 0; it < iterations; ++it) {
        // x 的每一段都在各自的 rank 上，先拼成完整向量。
        MPI_Allgather(local_x.data(), local_n, MPI_DOUBLE, x.data(), local_n,
                      MPI_DOUBLE, MPI_COMM_WORLD);

        for (int i = 0; i < local_n; ++i) {
            double sum = 0.0;
            for (int j = 0; j < n; ++j) {
                sum += a[static_cast<std::size_t>(i) * n + j] * x[j];
            }
            local_y[static_cast<std::size_t>(i)] = sum;
        }

        // 做一次归一化，避免三轮之后数值膨胀；归一化本身也是集合通信。
        double local_sum = 0.0;
        for (double v : local_y) {
            local_sum += v;
        }
        double global_sum = 0.0;
        MPI_Allreduce(&local_sum, &global_sum, 1, MPI_DOUBLE, MPI_SUM,
                      MPI_COMM_WORLD);
        for (int i = 0; i < local_n; ++i) {
            local_x[static_cast<std::size_t>(i)] =
                local_y[static_cast<std::size_t>(i)] / global_sum;
        }
    }

    const double t1 = MPI_Wtime();
    double local_time = t1 - t0;
    double slowest = 0.0;
    MPI_Reduce(&local_time, &slowest, 1, MPI_DOUBLE, MPI_MAX, 0,
               MPI_COMM_WORLD);

    double local_check = 0.0;
    for (double v : local_y) {
        local_check += v;
    }
    double checksum = 0.0;
    MPI_Reduce(&local_check, &checksum, 1, MPI_DOUBLE, MPI_SUM, 0,
               MPI_COMM_WORLD);

    if (rank == 0) {
        std::printf("n=%d p=%d 迭代=%d\n", n, size, iterations);
        std::printf("校验和: %.0f\n", checksum);
        std::printf("最慢 rank 用时: %.3f ms\n", slowest * 1000.0);
    }

    MPI_Finalize();
    return 0;
}
// ANCHOR_END: core
