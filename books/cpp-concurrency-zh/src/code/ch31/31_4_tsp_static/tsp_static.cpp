// 12 城市 TSP：rank 0 用 BFS 生成任务，MPI_Scatterv 分发，各 rank 做 DFS
#include <mpi.h>

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <vector>

namespace {
constexpr int N = 12;
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
    std::vector<int> best_path;
    long long nodes = 0;

    void dfs(const Task& t) {
        ++nodes;
        if (t.cost >= best) return;
        if (t.len == N) {
            const int last = t.path[N - 1];
            const int total = t.cost + dist[last * N];
            if (total < best) {
                best = total;
                best_path.assign(t.path, t.path + N);
                best_path.push_back(0);
            }
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

    // 12 个城市的坐标；rank 0 建距离矩阵后广播
    const int px[N] = {22, 41, 98, 8, 48, 99, 20, 92, 56, 79, 30, 40};
    const int py[N] = {77, 97, 97, 71, 1, 62, 79, 29, 7, 76, 63, 68};
    if (rank == 0) {
        for (int i = 0; i < N; ++i)
            for (int j = 0; j < N; ++j)
                dist[i * N + j] =
                    static_cast<int>(std::lround(std::hypot(px[i] - px[j], py[i] - py[j])));
    }
    MPI_Bcast(dist, N * N, MPI_INT, 0, MPI_COMM_WORLD);

    // 最近邻回路作为初始上界，顺便给每个 rank 一条合法回路
    std::vector<int> greedy;
    int greedy_cost = 0;
    if (rank == 0) {
        std::vector<char> used(N, 0);
        int u = 0;
        greedy.push_back(0);
        used[0] = 1;
        for (int k = 1; k < N; ++k) {
            int best_v = -1, best_d = 1 << 30;
            for (int v = 0; v < N; ++v) {
                if (used[v]) continue;
                if (dist[u * N + v] < best_d) {
                    best_d = dist[u * N + v];
                    best_v = v;
                }
            }
            used[best_v] = 1;
            greedy_cost += best_d;
            greedy.push_back(best_v);
            u = best_v;
        }
        greedy_cost += dist[u * N];
        greedy.push_back(0);
    }
    MPI_Bcast(&greedy_cost, 1, MPI_INT, 0, MPI_COMM_WORLD);
    greedy.resize(N + 1);
    MPI_Bcast(greedy.data(), N + 1, MPI_INT, 0, MPI_COMM_WORLD);

    // rank 0 做 BFS，生成至少 p 条、长度相同的部分回路
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

    // 条数不一定被 p 整除：用 Scatterv，counts/displacements 以 int 为单位
    // ANCHOR: scatterv
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
    // ANCHOR_END: scatterv

    const int mine = counts[rank] / WORDS;
    Solver solver;
    solver.best = greedy_cost;
    solver.best_path = greedy;
    for (int i = 0; i < mine; ++i) solver.dfs(unflatten(&recvbuf[i * WORDS]));

    // ANCHOR: minloc
    struct BestPair {
        int cost;
        int rank;
    } local{solver.best, rank}, global{};
    MPI_Allreduce(&local, &global, 1, MPI_2INT, MPI_MINLOC, MPI_COMM_WORLD);
    // ANCHOR_END: minloc

    std::vector<int> final_path(N + 1);
    if (global.rank == 0) {
        final_path = solver.best_path;
    } else if (rank == global.rank) {
        MPI_Send(solver.best_path.data(), N + 1, MPI_INT, 0, 77, MPI_COMM_WORLD);
    } else if (rank == 0) {
        MPI_Recv(final_path.data(), N + 1, MPI_INT, global.rank, 77, MPI_COMM_WORLD,
                 MPI_STATUS_IGNORE);
    }

    long long local_nodes = solver.nodes, total_nodes = 0;
    MPI_Reduce(&local_nodes, &total_nodes, 1, MPI_LONG_LONG, MPI_SUM, 0,
               MPI_COMM_WORLD);
    std::vector<long long> all_nodes;
    if (rank == 0) all_nodes.resize(p);
    MPI_Gather(&local_nodes, 1, MPI_LONG_LONG, all_nodes.data(), 1, MPI_LONG_LONG,
               0, MPI_COMM_WORLD);

    if (rank == 0) {
        std::printf("城市数=%d，进程数=%d，BFS 任务=%d\n", N, p, nt);
        std::printf("greedy=%d\n", greedy_cost);
        std::printf("best_cost=%d\n", global.cost);
        std::printf("best_tour=");
        for (int v : final_path) std::printf(" %d", v);
        std::printf("\nnodes_per_rank=");
        for (int q = 0; q < p; ++q)
            std::printf("%s%lld", q == 0 ? "" : " ", all_nodes[q]);
        std::printf("\ntotal_nodes=%lld\n", total_nodes);
    }
    MPI_Finalize();
    return 0;
}
