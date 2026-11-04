// 基准：n=65536 的 MPI 奇偶排序，时间包含分发、本地排序、p 个阶段和回收
#include <mpi.h>

#include <algorithm>
#include <cstdio>
#include <vector>

static void merge_low(const std::vector<int>& a, const std::vector<int>& b,
                      std::vector<int>& out) {
    out.clear();
    std::size_t i = 0, j = 0;
    while (out.size() < a.size()) {
        if (j >= b.size() || (i < a.size() && a[i] <= b[j]))
            out.push_back(a[i++]);
        else
            out.push_back(b[j++]);
    }
}

static void merge_high(const std::vector<int>& lo, const std::vector<int>& hi,
                       std::vector<int>& out) {
    out.assign(lo.size(), 0);
    std::size_t i = lo.size(), j = hi.size(), k = out.size();
    while (k > 0) {
        if (j == 0 || (i > 0 && lo[i - 1] >= hi[j - 1]))
            out[--k] = lo[--i];
        else
            out[--k] = hi[--j];
    }
}

int main(int argc, char** argv) {
    MPI_Init(&argc, &argv);
    int rank = 0, p = 0;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &p);

    constexpr int n = 65536;
    const int loc_n = n / p;
    std::vector<int> all;
    if (rank == 0) {
        all.resize(n);
        for (int i = 0; i < n; ++i) all[i] = n - i;
    }
    std::vector<int> local(loc_n);
    std::vector<int> recv(loc_n), next(loc_n), gathered;

    MPI_Barrier(MPI_COMM_WORLD);
    const double t0 = MPI_Wtime();
    MPI_Scatter(all.data(), loc_n, MPI_INT, local.data(), loc_n, MPI_INT, 0,
                MPI_COMM_WORLD);
    std::sort(local.begin(), local.end());

    for (int phase = 0; phase < p; ++phase) {
        int partner = MPI_PROC_NULL;
        if (phase % 2 == 0)
            partner = rank ^ 1;
        else if (rank % 2 == 1)
            partner = rank + 1;
        else
            partner = rank - 1;
        if (partner < 0 || partner >= p) partner = MPI_PROC_NULL;
        if (partner == MPI_PROC_NULL) continue;
        MPI_Sendrecv(local.data(), loc_n, MPI_INT, partner, 0, recv.data(), loc_n,
                     MPI_INT, partner, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        if (rank < partner)
            merge_low(local, recv, next);
        else
            merge_high(recv, local, next);
        local.swap(next);
    }

    if (rank == 0) gathered.resize(n);
    MPI_Gather(local.data(), loc_n, MPI_INT, gathered.data(), loc_n, MPI_INT, 0,
               MPI_COMM_WORLD);
    const double t1 = MPI_Wtime();

    double local_t = t1 - t0, max_t = 0.0;
    MPI_Reduce(&local_t, &max_t, 1, MPI_DOUBLE, MPI_MAX, 0, MPI_COMM_WORLD);

    if (rank == 0) {
        unsigned long long sum = 0;
        for (int i = 0; i < n; ++i)
            sum += static_cast<unsigned long long>(i + 1) * gathered[i];
        std::printf("sort procs=%d time_ms=%.3f checksum=%llu\n", p,
                    1000.0 * max_t, sum);
    }
    MPI_Finalize();
    return 0;
}
