// 基准：13 城市 TSP 的静态划分，时间只含各 rank 的 DFS
#include <mpi.h>

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <vector>

namespace {
constexpr int N = 13;
constexpr int MAXC = 16;
constexpr int WORDS = 3 + MAXC;
constexpr int BFS_DEPTH = 4;

struct Task {
    int len = 1;
    int path[MAXC]{};
    int cost = 0;
    int mask = 1;
};

int dist[N * N];

void flatten(const Task& t, int* out) {
    out[0] = t.len;
    out[1] = t.cost;
    out[2] = t.mask;
    for (int i = 0; i < MAXC; ++i) out[3 + i] = t.path[i];
}

Task unflatten(const int* in) {
    Task t;
    t.len = in[0];
    t.cost = in[1];
    t.mask = in[2];
    for (int i = 0; i < MAXC; ++i) t.path[i] = in[3 + i];
    return t;
}

struct Solver {
    int best = 0;
    long long nodes = 0;

    void dfs(const Task& t) {
        ++nodes;
        if (t.cost >= best) return;
        if (t.len == N) {
            const int total = t.cost + dist[t.path[N - 1] * N];
            if (total < best) best = total;
            return;
        }
        const int last = t.path[t.len - 1];
        for (int v = 1; v < N; ++v) {
            if (t.mask & (1 << v)) continue;
            const int nc = t.cost + dist[last * N + v];
            if (nc >= best) continue;
            Task nt = t;
            nt.path[nt.len++] = v;
            nt.cost = nc;
            nt.mask |= 1 << v;
            dfs(nt);
        }
    }
};
}  // namespace

int main(int argc, char** argv) {
    MPI_Init(&argc, &argv);
    int rank = 0, p = 0;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &p);

    const int px[N] = {22, 41, 98, 8, 48, 99, 20, 92, 56, 79, 30, 40, 90};
    const int py[N] = {77, 97, 97, 71, 1, 62, 79, 29, 7, 76, 63, 68, 87};
    if (rank == 0) {
        for (int i = 0; i < N; ++i)
            for (int j = 0; j < N; ++j)
                dist[i * N + j] =
                    static_cast<int>(std::lround(std::hypot(px[i] - px[j], py[i] - py[j])));
    }
    MPI_Bcast(dist, N * N, MPI_INT, 0, MPI_COMM_WORLD);

    std::vector<int> greedy;
    int greedy_cost = 0;
    if (rank == 0) {
        std::vector<char> used(N, 0);
        int u = 0;
        greedy.push_back(0);
        used[0] = 1;
        for (int k = 1; k < N; ++k) {
            int bv = -1, bd = 1 << 30;
            for (int v = 0; v < N; ++v) {
                if (used[v]) continue;
                if (dist[u * N + v] < bd) {
                    bd = dist[u * N + v];
                    bv = v;
                }
            }
            used[bv] = 1;
            greedy_cost += bd;
            greedy.push_back(bv);
            u = bv;
        }
        greedy_cost += dist[u * N];
        greedy.push_back(0);
    }
    MPI_Bcast(&greedy_cost, 1, MPI_INT, 0, MPI_COMM_WORLD);
    greedy.resize(N + 1);
    MPI_Bcast(greedy.data(), N + 1, MPI_INT, 0, MPI_COMM_WORLD);

    std::vector<Task> tasks;
    if (rank == 0) {
        std::vector<Task> q;
        Task root;
        root.path[0] = 0;
        q.push_back(root);
        for (std::size_t head = 0; head < q.size(); ++head) {
            Task t = q[head];
            if (t.len == BFS_DEPTH + 1) {
                tasks.push_back(t);
                continue;
            }
            const int last = t.path[t.len - 1];
            for (int v = 1; v < N; ++v) {
                if (t.mask & (1 << v)) continue;
                Task nt = t;
                nt.path[nt.len++] = v;
                nt.cost += dist[last * N + v];
                nt.mask |= 1 << v;
                q.push_back(nt);
            }
        }
    }

    int nt = static_cast<int>(tasks.size());
    MPI_Bcast(&nt, 1, MPI_INT, 0, MPI_COMM_WORLD);
    std::vector<int> counts(p), displs(p);
    const int base = nt / p, rem = nt % p;
    int offset = 0;
    for (int q = 0; q < p; ++q) {
        const int nq = base + (q < rem ? 1 : 0);
        counts[q] = nq * WORDS;
        displs[q] = offset;
        offset += nq * WORDS;
    }
    std::vector<int> sendbuf;
    if (rank == 0) {
        sendbuf.resize(nt * WORDS);
        for (int i = 0; i < nt; ++i) flatten(tasks[i], &sendbuf[i * WORDS]);
    }
    const int max_count = (base + (rem > 0 ? 1 : 0)) * WORDS;
    std::vector<int> recvbuf(std::max(max_count, WORDS));
    MPI_Scatterv(sendbuf.data(), counts.data(), displs.data(), MPI_INT,
                 recvbuf.data(), max_count, MPI_INT, 0, MPI_COMM_WORLD);

    Solver solver;
    solver.best = greedy_cost;
    const int mine = counts[rank] / WORDS;

    MPI_Barrier(MPI_COMM_WORLD);
    const double t0 = MPI_Wtime();
    for (int i = 0; i < mine; ++i) solver.dfs(unflatten(&recvbuf[i * WORDS]));
    const double t1 = MPI_Wtime();

    struct BestPair {
        int cost;
        int rank;
    } local{solver.best, rank}, global{};
    MPI_Allreduce(&local, &global, 1, MPI_2INT, MPI_MINLOC, MPI_COMM_WORLD);

    double local_t = t1 - t0, max_t = 0.0;
    MPI_Reduce(&local_t, &max_t, 1, MPI_DOUBLE, MPI_MAX, 0, MPI_COMM_WORLD);
    if (rank == 0)
        std::printf("tsp procs=%d time_ms=%.3f checksum=%d\n", p, 1000.0 * max_t,
                    global.cost);
    MPI_Finalize();
    return 0;
}
