// mode: mpi
// expect: rank 3：9 个 double，总和 27036
#include <mpi.h>

#include <cstdio>
#include <vector>

int main(int argc, char **argv) {
    MPI_Init(&argc, &argv);

    int rank = -1;
    int size = 0;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    if (rank != 0) {
        const int n = rank * 3;
        std::vector<double> values(static_cast<std::size_t>(n));
        for (int i = 0; i < n; ++i) {
            values[static_cast<std::size_t>(i)] = rank * 1000.0 + i;
        }
        MPI_Send(values.data(), n, MPI_DOUBLE, 0, 5, MPI_COMM_WORLD);
    } else {
        for (int src = 1; src < size; ++src) {
            MPI_Status status;
            // Probe 只读信封，消息还留在队列里。
            MPI_Probe(src, 5, MPI_COMM_WORLD, &status);

            int count = 0;
            int elements = 0;
            MPI_Get_count(&status, MPI_DOUBLE, &count);
            MPI_Get_elements(&status, MPI_DOUBLE, &elements);

            std::vector<double> values(static_cast<std::size_t>(count));
            MPI_Recv(values.data(), count, MPI_DOUBLE, src, 5, MPI_COMM_WORLD,
                     MPI_STATUS_IGNORE);

            double sum = 0.0;
            for (double value : values) {
                sum += value;
            }
            std::printf("rank %d：%d 个 double，总和 %.0f（Get_elements=%d）\n",
                        src, count, sum, elements);
        }
    }

    MPI_Finalize();
    return 0;
}
