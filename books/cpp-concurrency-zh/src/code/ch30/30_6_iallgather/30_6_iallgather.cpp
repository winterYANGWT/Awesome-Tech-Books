// 阻塞版先 Allgather 再计算；非阻塞版先提交 Iallgather，再计算，最后 Wait。
#include <mpi.h>

#include <cstdio>
#include <vector>

// ANCHOR: core
namespace {
volatile double g_sink = 0.0;

double local_work(double seed, int iterations) {
    double acc = seed;
    for (int k = 0; k < iterations; ++k) {
        acc += static_cast<double>((k * 7 + 3) % 13) * 0.25;
    }
    g_sink += acc;
    return acc;
}
}  // namespace

int main(int argc, char** argv) {
    MPI_Init(&argc, &argv);

    int rank = 0;
    int size = 0;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    const int block = 1 << 18;  // 每个 rank 2 MiB 的 double
    std::vector<double> local(static_cast<std::size_t>(block));
    for (int i = 0; i < block; ++i) {
        local[static_cast<std::size_t>(i)] =
            static_cast<double>(rank) * 1000.0 + static_cast<double>(i);
    }
    std::vector<double> full(static_cast<std::size_t>(block) * size, 0.0);

    const int reps = 3;
    double blocking_best = 1e100;
    double overlap_best = 1e100;
    double work_value = 0.0;

    for (int rep = 0; rep < reps; ++rep) {
        MPI_Barrier(MPI_COMM_WORLD);
        double t0 = MPI_Wtime();
        MPI_Allgather(local.data(), block, MPI_DOUBLE, full.data(), block,
                      MPI_DOUBLE, MPI_COMM_WORLD);
        work_value = local_work(static_cast<double>(rank + rep), 20'000'000);
        double t1 = MPI_Wtime();
        if (t1 - t0 < blocking_best) {
            blocking_best = t1 - t0;
        }

        MPI_Barrier(MPI_COMM_WORLD);
        t0 = MPI_Wtime();
        MPI_Request req;
        MPI_Iallgather(local.data(), block, MPI_DOUBLE, full.data(), block,
                       MPI_DOUBLE, MPI_COMM_WORLD, &req);
        work_value += local_work(static_cast<double>(rank + rep), 20'000'000);
        MPI_Wait(&req, MPI_STATUS_IGNORE);
        t1 = MPI_Wtime();
        if (t1 - t0 < overlap_best) {
            overlap_best = t1 - t0;
        }
    }

    double local_sum = work_value;
    for (double v : full) {
        local_sum += v;
    }
    double checksum = 0.0;
    MPI_Reduce(&local_sum, &checksum, 1, MPI_DOUBLE, MPI_SUM, 0,
               MPI_COMM_WORLD);
    double local_block = blocking_best;
    double local_overlap = overlap_best;
    double block_max = 0.0;
    double overlap_max = 0.0;
    MPI_Reduce(&local_block, &block_max, 1, MPI_DOUBLE, MPI_MAX, 0,
               MPI_COMM_WORLD);
    MPI_Reduce(&local_overlap, &overlap_max, 1, MPI_DOUBLE, MPI_MAX, 0,
               MPI_COMM_WORLD);

    if (rank == 0) {
        std::printf("校验和: %.3f\n", checksum);
        std::printf("阻塞版: %.3f ms\n", block_max * 1000.0);
        std::printf("非阻塞版: %.3f ms\n", overlap_max * 1000.0);
        std::printf("阻塞/非阻塞: %.3f\n", block_max / overlap_max);
    }

    MPI_Finalize();
    return 0;
}
// ANCHOR_END: core
