// mode: mpi
// np: 4
// args: 0 1 1000000
// expect: 相对误差
#include <mpi.h>
#include <stdio.h>

static double f(double x) {
    return 4.0 / (1.0 + x * x);
}

int main(int argc, char **argv) {
    MPI_Init(&argc, &argv);

    int rank = -1;
    int size = 0;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    double a = 0.0;
    double b = 1.0;
    long n = 1000000;
    if (rank == 0 && argc >= 4) {
        if (sscanf(argv[1], "%lf", &a) != 1 ||
            sscanf(argv[2], "%lf", &b) != 1 ||
            sscanf(argv[3], "%ld", &n) != 1 || n <= 0) {
            fprintf(stderr, "用法：prog a b n\n");
            MPI_Abort(MPI_COMM_WORLD, 1);
        }
    }
    MPI_Bcast(&a, 1, MPI_DOUBLE, 0, MPI_COMM_WORLD);
    MPI_Bcast(&b, 1, MPI_DOUBLE, 0, MPI_COMM_WORLD);
    MPI_Bcast(&n, 1, MPI_LONG, 0, MPI_COMM_WORLD);

    const double h = (b - a) / (double)n;
    const long quot = n / size;
    const long rem = n % size;
    const long local_n = quot + (rank < rem ? 1 : 0);
    const long offset = rank * quot + (rank < rem ? rank : rem);
    const double local_a = a + (double)offset * h;
    const double local_b = local_a + (double)local_n * h;

    double local_sum = 0.0;
    if (local_n > 0) {
        local_sum = (f(local_a) + f(local_b)) / 2.0;
        for (long i = 1; i < local_n; ++i) {
            local_sum += f(local_a + (double)i * h);
        }
        local_sum *= h;
    }

    if (rank != 0) {
        MPI_Send(&local_sum, 1, MPI_DOUBLE, 0, 4, MPI_COMM_WORLD);
    } else {
        double total = local_sum;
        for (int src = 1; src < size; ++src) {
            double part = 0.0;
            MPI_Recv(&part, 1, MPI_DOUBLE, src, 4, MPI_COMM_WORLD,
                     MPI_STATUS_IGNORE);
            total += part;
        }
        const double pi = 3.14159265358979323846;
        printf("梯形积分：p=%d，n=%ld，每个进程 %ld 个梯形\n", size, n,
               local_n);
        printf("pi ≈ %.15g\n", total);
        printf("相对误差 %.2e\n", (total - pi) / pi);
    }

    MPI_Finalize();
    return 0;
}
