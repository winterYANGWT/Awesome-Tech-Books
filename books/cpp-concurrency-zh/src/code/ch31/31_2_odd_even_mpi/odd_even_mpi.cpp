// n=16、p=4：每个进程先排本地块，再做 p 个阶段的 compare-split
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
    std::size_t i = lo.size(), j = hi.size();
    std::size_t k = out.size();
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

    constexpr int n = 16;
    if (n % p != 0) {
        if (rank == 0) std::printf("n=%d 不能被 p=%d 整除\n", n, p);
        MPI_Finalize();
        return 1;
    }
    const int loc_n = n / p;

    std::vector<int> all;
    if (rank == 0) {
        all.resize(n);
        for (int i = 0; i < n; ++i) all[i] = n - i;  // 逆序，最坏情况
    }
    std::vector<int> local(loc_n);
    MPI_Scatter(all.data(), loc_n, MPI_INT, local.data(), loc_n, MPI_INT, 0,
                MPI_COMM_WORLD);
    std::sort(local.begin(), local.end());

    std::vector<int> recv(loc_n), next(loc_n);
    // ANCHOR: phase
    for (int phase = 0; phase < p; ++phase) {
        int partner = MPI_PROC_NULL;
        if (phase % 2 == 0) {
            partner = rank ^ 1;
        } else if (rank % 2 == 1) {
            partner = rank + 1;
        } else {
            partner = rank - 1;
        }
        if (partner < 0 || partner >= p) partner = MPI_PROC_NULL;

        if (partner != MPI_PROC_NULL) {
            // 一次调用同时发和收，MPI 负责调度，避免所有 rank 先发后收
            MPI_Sendrecv(local.data(), loc_n, MPI_INT, partner, 0, recv.data(),
                         loc_n, MPI_INT, partner, 0, MPI_COMM_WORLD,
                         MPI_STATUS_IGNORE);
            if (rank < partner)
                merge_low(local, recv, next);
            else
                merge_high(recv, local, next);
            local.swap(next);
        }
    }
    // ANCHOR_END: phase

    std::vector<int> gathered;
    if (rank == 0) gathered.resize(n);
    MPI_Gather(local.data(), loc_n, MPI_INT, gathered.data(), loc_n, MPI_INT, 0,
               MPI_COMM_WORLD);

    int status = 0;
    if (rank == 0) {
        bool sorted = std::is_sorted(gathered.begin(), gathered.end());
        std::printf("p=%d，n=%d，每块 %d 个键值\n", p, n, loc_n);
        std::printf("排序结果:");
        for (int x : gathered) std::printf(" %d", x);
        std::printf("\n全局有序: %s\n", sorted ? "是" : "否");
        status = sorted ? 0 : 1;
    }
    MPI_Finalize();
    return status;
}
