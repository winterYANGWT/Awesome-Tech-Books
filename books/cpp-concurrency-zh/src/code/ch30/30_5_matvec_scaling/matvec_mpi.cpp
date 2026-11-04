// MPI 版：按行块划分 A 和 y；每次乘法前用 Allgather 拼出完整 x。
#include <mpi.h>

#include <cstdio>
#include <cstdlib>
#include <vector>

int main(int argc, char** argv) {
    MPI_Init(&argc, &argv);

    int rank = 0;
    int size = 0;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    const int n = argc > 1 ? std::atoi(argv[1]) : 8192;
    if (n % size != 0) {
        if (rank == 0) {
            std::fprintf(stderr, "n=%d 不能被 p=%d 整除\n", n, size);
        }
        MPI_Finalize();
        return 1;
    }
    const int local_n = n / size;
    const int row0 = rank * local_n;
    const int reps = 5;

    std::vector<double> a(static_cast<std::size_t>(local_n) * n);
    for (int i = 0; i < local_n; ++i) {
        for (int j = 0; j < n; ++j) {
            a[static_cast<std::size_t>(i) * n + j] =
                static_cast<double>((row0 + i + j) % 7) * 0.5;
        }
    }

    std::vector<double> local_x(static_cast<std::size_t>(local_n), 1.0);
    std::vector<double> x(static_cast<std::size_t>(n), 0.0);
    std::vector<double> local_y(static_cast<std::size_t>(local_n), 0.0);

    double best = 1e100;
    for (int rep = 0; rep < reps; ++rep) {
        MPI_Barrier(MPI_COMM_WORLD);
        const double t0 = MPI_Wtime();

        MPI_Allgather(local_x.data(), local_n, MPI_DOUBLE, x.data(), local_n,
                      MPI_DOUBLE, MPI_COMM_WORLD);

        for (int i = 0; i < local_n; ++i) {
            double sum = 0.0;
            for (int j = 0; j < n; ++j) {
                sum += a[static_cast<std::size_t>(i) * n + j] * x[j];
            }
            local_y[static_cast<std::size_t>(i)] = sum;
        }

        const double elapsed = MPI_Wtime() - t0;
        if (elapsed < best) {
            best = elapsed;
        }
    }

    double local_check = 0.0;
    for (double v : local_y) {
        local_check += v;
    }
    double checksum = 0.0;
    MPI_Reduce(&local_check, &checksum, 1, MPI_DOUBLE, MPI_SUM, 0,
               MPI_COMM_WORLD);

    // CSV 只由 rank 0 写 stdout；校验和等日志走 stderr。
    if (rank == 0) {
        std::fprintf(stderr, "p=%d checksum=%.3f\n", size, checksum);
        std::printf("%d,%.3f\n", size, best * 1000.0);
    }

    MPI_Finalize();
    return 0;
}
