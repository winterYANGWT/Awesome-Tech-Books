// MPI 管节点间，OpenMP 管节点内；只请求 MPI_THREAD_FUNNELED。
#include <mpi.h>
#include <omp.h>

#include <cstdio>

// ANCHOR: core
int main(int argc, char** argv) {
    int provided = MPI_THREAD_SINGLE;
    MPI_Init_thread(&argc, &argv, MPI_THREAD_FUNNELED, &provided);

    int rank = 0;
    int size = 0;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    const long long n = 10'000'000;
    const long long begin = static_cast<long long>(rank) * n;
    long long local = 0;
#pragma omp parallel for reduction(+ : local)
    for (long long i = 0; i < n; ++i) {
        local += begin + i;
    }

    long long total = 0;
    MPI_Reduce(&local, &total, 1, MPI_LONG_LONG, MPI_SUM, 0, MPI_COMM_WORLD);

    const char* level = "SINGLE";
    if (provided == MPI_THREAD_FUNNELED) {
        level = "FUNNELED";
    } else if (provided == MPI_THREAD_SERIALIZED) {
        level = "SERIALIZED";
    } else if (provided == MPI_THREAD_MULTIPLE) {
        level = "MULTIPLE";
    }

    if (rank == 0) {
        std::printf("requested=FUNNELED provided=%s(%d)\n", level, provided);
        std::printf("omp_get_max_threads()=%d\n", omp_get_max_threads());
        std::printf("总校验和=%lld\n", total);
    }

    MPI_Finalize();
    return 0;
}
// ANCHOR_END: core
