// mode: mpi
// flags: -fopenmp
// np: 4
// tsan: no（MPI 程序不在 TSan 下运行）
// expect: provided=
#include <mpi.h>
#include <omp.h>

#include <cstdio>

static const char *level_name(int level) {
    switch (level) {
    case MPI_THREAD_SINGLE:
        return "MPI_THREAD_SINGLE";
    case MPI_THREAD_FUNNELED:
        return "MPI_THREAD_FUNNELED";
    case MPI_THREAD_SERIALIZED:
        return "MPI_THREAD_SERIALIZED";
    case MPI_THREAD_MULTIPLE:
        return "MPI_THREAD_MULTIPLE";
    default:
        return "未知";
    }
}

int main(int argc, char **argv) {
    int provided = -1;
    MPI_Init_thread(&argc, &argv, MPI_THREAD_FUNNELED, &provided);

    int rank = -1;
    int size = 0;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    long long local = 0;
    int threads = 0;
    const long long begin = rank * 1000000LL;

#pragma omp parallel num_threads(2) reduction(+ : local)
    {
#pragma omp atomic
        threads += 1;
#pragma omp for
        for (long long i = 0; i < 1000000LL; ++i) {
            local += begin + i;
        }
    }

    if (rank != 0) {
        MPI_Send(&provided, 1, MPI_INT, 0, 3, MPI_COMM_WORLD);
        MPI_Send(&threads, 1, MPI_INT, 0, 3, MPI_COMM_WORLD);
        MPI_Send(&local, 1, MPI_LONG_LONG, 0, 3, MPI_COMM_WORLD);
    } else {
        printf("rank 0：provided=%d（%s），OpenMP 线程 %d，局部和 %lld\n",
               provided, level_name(provided), threads, local);
        for (int src = 1; src < size; ++src) {
            int src_provided = -1;
            int src_threads = 0;
            long long src_local = 0;
            MPI_Recv(&src_provided, 1, MPI_INT, src, 3, MPI_COMM_WORLD,
                     MPI_STATUS_IGNORE);
            MPI_Recv(&src_threads, 1, MPI_INT, src, 3, MPI_COMM_WORLD,
                     MPI_STATUS_IGNORE);
            MPI_Recv(&src_local, 1, MPI_LONG_LONG, src, 3, MPI_COMM_WORLD,
                     MPI_STATUS_IGNORE);
            printf("rank %d：provided=%d（%s），OpenMP 线程 %d，局部和 %lld\n",
                   src, src_provided, level_name(src_provided), src_threads,
                   src_local);
        }
    }

    MPI_Finalize();
    return 0;
}
