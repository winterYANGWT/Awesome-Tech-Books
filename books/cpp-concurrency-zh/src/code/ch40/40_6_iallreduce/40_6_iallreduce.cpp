#include <mpi.h>

#include <cstdio>
#include <vector>

namespace {
volatile long long g_sink = 0;

long long local_work(long long seed, int iterations) {
    long long acc = seed;
    for (int k = 0; k < iterations; ++k) {
        acc += static_cast<long long>((k * 13 + 7) % 17);
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

    int major = 0;
    int minor = 0;
    MPI_Get_version(&major, &minor);

    constexpr int kBlock = 1 << 20;  // 每个 rank 8 MiB 的 long long
    std::vector<long long> local(static_cast<std::size_t>(kBlock));
    std::vector<long long> global(static_cast<std::size_t>(kBlock), 0);
    for (int i = 0; i < kBlock; ++i) {
        local[static_cast<std::size_t>(i)] =
            static_cast<long long>(rank) * kBlock + i + 1;
    }

    double blocking_best = 1e100;
    double overlap_best = 1e100;
    long long work_value = 0;

    for (int rep = 0; rep < 3; ++rep) {
        MPI_Barrier(MPI_COMM_WORLD);
        double start = MPI_Wtime();
        MPI_Allreduce(local.data(), global.data(), kBlock, MPI_LONG_LONG, MPI_SUM,
                      MPI_COMM_WORLD);
        work_value += local_work(rank + rep, 20'000'000);
        double stop = MPI_Wtime();
        if (stop - start < blocking_best) blocking_best = stop - start;

        MPI_Barrier(MPI_COMM_WORLD);
        start = MPI_Wtime();
        MPI_Request request;
        MPI_Iallreduce(local.data(), global.data(), kBlock, MPI_LONG_LONG, MPI_SUM,
                       MPI_COMM_WORLD, &request);
        work_value += local_work(rank + rep, 20'000'000);
        MPI_Wait(&request, MPI_STATUS_IGNORE);
        stop = MPI_Wtime();
        if (stop - start < overlap_best) overlap_best = stop - start;
    }

    // Allreduce 是逐元素求和：global[0] 是各 rank 的 local[0] 之和。
    const long long expected =
        static_cast<long long>(size) * (size - 1) / 2 * kBlock + size;
    const int local_ok = (global[0] == expected) ? 1 : 0;
    int all_ok = 0;
    MPI_Allreduce(&local_ok, &all_ok, 1, MPI_INT, MPI_MIN, MPI_COMM_WORLD);

    double blocking_max = 0.0;
    double overlap_max = 0.0;
    MPI_Reduce(&blocking_best, &blocking_max, 1, MPI_DOUBLE, MPI_MAX, 0, MPI_COMM_WORLD);
    MPI_Reduce(&overlap_best, &overlap_max, 1, MPI_DOUBLE, MPI_MAX, 0, MPI_COMM_WORLD);

    long long report[2] = {global[0], work_value};
    std::vector<long long> reports(static_cast<std::size_t>(2 * size), 0);
    MPI_Gather(report, 2, MPI_LONG_LONG, reports.data(), 2, MPI_LONG_LONG, 0,
               MPI_COMM_WORLD);

    if (rank == 0) {
        for (int r = 0; r < size; ++r) {
            std::printf("rank %d: allreduce_sum=%lld expected=%lld checksum=%lld\n", r,
                        reports[static_cast<std::size_t>(2 * r)], expected,
                        reports[static_cast<std::size_t>(2 * r + 1)]);
        }
        std::printf("mpi_version=%d.%d\n", major, minor);
        std::printf("allreduce_ok=%d\n", all_ok);
        std::printf("blocking_ms=%.3f\n", blocking_max * 1000.0);
        std::printf("overlap_ms=%.3f\n", overlap_max * 1000.0);
        std::printf("blocking_over_overlap=%.3f\n", blocking_max / overlap_max);
    }

    MPI_Finalize();
    return all_ok == 1 ? 0 : 1;
}
