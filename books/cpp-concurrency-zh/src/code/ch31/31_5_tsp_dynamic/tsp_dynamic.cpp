// 12 城市 TSP 的动态划分：空闲 rank 请求工作，能量守恒检测终止
#include <mpi.h>

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <thread>
#include <vector>

namespace {
constexpr int N = 12;
constexpr int MAXC = 16;
constexpr int WORDS = 3 + MAXC;
constexpr int CHUNK = 2000;
enum Tag { REQUEST = 10, WORK = 11, NO_WORK = 12, ENERGY = 13, TERMINATE = 14 };

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

// ANCHOR: pack
int pack_work(const std::vector<Task>& ts, double energy_in,
              std::vector<char>& buf) {
    int dbytes = 0, ibytes = 0;
    MPI_Pack_size(1, MPI_DOUBLE, MPI_COMM_WORLD, &dbytes);
    MPI_Pack_size(1 + static_cast<int>(ts.size()) * WORDS, MPI_INT,
                  MPI_COMM_WORLD, &ibytes);
    buf.resize(dbytes + ibytes);
    int pos = 0;
    MPI_Pack(&energy_in, 1, MPI_DOUBLE, buf.data(), static_cast<int>(buf.size()),
             &pos, MPI_COMM_WORLD);
    const int n = static_cast<int>(ts.size());
    MPI_Pack(&n, 1, MPI_INT, buf.data(), static_cast<int>(buf.size()), &pos,
             MPI_COMM_WORLD);
    std::vector<int> flat(ts.size() * WORDS);
    for (std::size_t i = 0; i < ts.size(); ++i)
        flatten(ts[i], &flat[i * WORDS]);
    MPI_Pack(flat.data(), static_cast<int>(flat.size()), MPI_INT, buf.data(),
             static_cast<int>(buf.size()), &pos, MPI_COMM_WORLD);
    return pos;
}
// ANCHOR_END: pack

void unpack_work(const std::vector<char>& buf, double& energy_out,
                 std::vector<Task>& ts) {
    int pos = 0;
    MPI_Unpack(buf.data(), static_cast<int>(buf.size()), &pos, &energy_out, 1,
               MPI_DOUBLE, MPI_COMM_WORLD);
    int count = 0;
    MPI_Unpack(buf.data(), static_cast<int>(buf.size()), &pos, &count, 1, MPI_INT,
               MPI_COMM_WORLD);
    std::vector<int> flat(static_cast<std::size_t>(count) * WORDS);
    MPI_Unpack(buf.data(), static_cast<int>(buf.size()), &pos, flat.data(),
               static_cast<int>(flat.size()), MPI_INT, MPI_COMM_WORLD);
    ts.clear();
    for (int i = 0; i < count; ++i) ts.push_back(unflatten(&flat[i * WORDS]));
}

void send_work(int dest, const std::vector<Task>& ts, double e) {
    std::vector<char> buf;
    const int bytes = pack_work(ts, e, buf);
    MPI_Send(buf.data(), bytes, MPI_PACKED, dest, WORK, MPI_COMM_WORLD);
}
}  // namespace

int main(int argc, char** argv) {
    MPI_Init(&argc, &argv);
    int rank = 0, p = 0;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &p);

    const int px[N] = {22, 41, 98, 8, 48, 99, 20, 92, 56, 79, 30, 40};
    const int py[N] = {77, 97, 97, 71, 1, 62, 79, 29, 7, 76, 63, 68};
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

    int best = greedy_cost;
    std::vector<int> best_path = greedy;
    long long nodes = 0, requests = 0, splits = 0;
    double energy = 1.0, total_energy = 0.0;
    bool done = false;
    std::vector<Task> stack;
    if (rank == 0) {
        Task root;
        root.path[0] = 0;
        stack.push_back(root);
    }

    auto take_best = [&](const Task& t) {
        const int total = t.cost + dist[t.path[N - 1] * N];
        if (total < best) {
            best = total;
            best_path.assign(t.path, t.path + N);
            best_path.push_back(0);
        }
    };

    // 处理一条已到达的消息；返回是否处理了消息
    auto handle_one = [&]() -> bool {
        int flag = 0;
        MPI_Status st;
        MPI_Iprobe(MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &flag, &st);
        if (!flag) return false;
        const int src = st.MPI_SOURCE, tag = st.MPI_TAG;
        // ANCHOR: request
        if (tag == REQUEST) {
            int dummy = 0;
            MPI_Recv(&dummy, 1, MPI_INT, src, REQUEST, MPI_COMM_WORLD,
                     MPI_STATUS_IGNORE);
            ++requests;
            if (stack.size() >= 2) {
                const std::size_t half = stack.size() / 2;
                std::vector<Task> give(stack.end() - half, stack.end());
                stack.resize(stack.size() - half);
                const double give_e = energy / 2.0;
                energy -= give_e;
                send_work(src, give, give_e);
                ++splits;
            } else {
                MPI_Send(&dummy, 1, MPI_INT, src, NO_WORK, MPI_COMM_WORLD);
            }
        }
        // ANCHOR_END: request
        else if (tag == WORK) {
            int bytes = 0;
            MPI_Get_count(&st, MPI_PACKED, &bytes);
            std::vector<char> buf(bytes);
            MPI_Recv(buf.data(), bytes, MPI_PACKED, src, WORK, MPI_COMM_WORLD,
                     MPI_STATUS_IGNORE);
            double e = 0.0;
            std::vector<Task> ts;
            unpack_work(buf, e, ts);
            energy += e;
            for (const Task& t : ts) stack.push_back(t);
        } else if (tag == NO_WORK || tag == TERMINATE) {
            int dummy = 0;
            MPI_Recv(&dummy, 1, MPI_INT, src, tag, MPI_COMM_WORLD,
                     MPI_STATUS_IGNORE);
            if (tag == TERMINATE) done = true;
        } else if (tag == ENERGY) {
            double e = 0.0;
            MPI_Recv(&e, 1, MPI_DOUBLE, src, ENERGY, MPI_COMM_WORLD,
                     MPI_STATUS_IGNORE);
            total_energy += e;
        } else {
            int dummy = 0;
            MPI_Recv(&dummy, 1, MPI_INT, src, tag, MPI_COMM_WORLD,
                     MPI_STATUS_IGNORE);
        }
        return true;
    };

    auto drain = [&]() {
        while (handle_one()) {
        }
    };

    auto search_chunk = [&]() {
        int worked = 0;
        while (!stack.empty() && worked < CHUNK) {
            Task t = stack.back();
            stack.pop_back();
            ++worked;
            ++nodes;
            if (t.cost >= best) continue;
            if (t.len == N) {
                take_best(t);
                continue;
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
                stack.push_back(nt);
            }
        }
    };

    int idle_round = 0;
    while (!done) {
        if (!stack.empty()) {
            search_chunk();
            drain();
            if (rank == 0) {
                if (stack.empty()) {
                    total_energy += energy;
                    energy = 0.0;
                }
                if (stack.empty() && total_energy >= p - 1e-9) {
                    for (int q = 1; q < p; ++q) {
                        int dummy = 0;
                        MPI_Send(&dummy, 1, MPI_INT, q, TERMINATE, MPI_COMM_WORLD);
                    }
                    done = true;
                }
            }
            continue;
        }

        // ANCHOR: energy
        // 自己没工作了：把能量交给 rank 0（rank 0 记在自己账上）
        if (energy > 0.0) {
            if (rank == 0) {
                total_energy += energy;
                energy = 0.0;
            } else {
                MPI_Send(&energy, 1, MPI_DOUBLE, 0, ENERGY, MPI_COMM_WORLD);
                energy = 0.0;
            }
        }

        // ANCHOR_END: energy
        if (rank == 0) {
            drain();
            if (total_energy >= p - 1e-9) {
                for (int q = 1; q < p; ++q) {
                    int dummy = 0;
                    MPI_Send(&dummy, 1, MPI_INT, q, TERMINATE, MPI_COMM_WORLD);
                }
                done = true;
                break;
            }
            std::this_thread::sleep_for(std::chrono::microseconds(200));
            continue;
        }

        // 空闲 rank：向轮转到的目标请求工作
        int target = (rank + 1 + idle_round) % p;
        if (target == rank) target = (target + 1) % p;
        ++idle_round;
        int dummy = 0;
        MPI_Send(&dummy, 1, MPI_INT, target, REQUEST, MPI_COMM_WORLD);
        while (!done) {
            if (handle_one()) break;
            std::this_thread::sleep_for(std::chrono::microseconds(200));
        }
    }

    struct BestPair {
        int cost;
        int rank;
    } local{best, rank}, global{};
    MPI_Allreduce(&local, &global, 1, MPI_2INT, MPI_MINLOC, MPI_COMM_WORLD);

    std::vector<int> final_path(N + 1);
    if (global.rank == 0) {
        final_path = best_path;
    } else if (rank == global.rank) {
        MPI_Send(best_path.data(), N + 1, MPI_INT, 0, 77, MPI_COMM_WORLD);
    } else if (rank == 0) {
        MPI_Recv(final_path.data(), N + 1, MPI_INT, global.rank, 77, MPI_COMM_WORLD,
                 MPI_STATUS_IGNORE);
    }

    long long sum_req = 0, sum_split = 0, sum_nodes = 0;
    MPI_Reduce(&requests, &sum_req, 1, MPI_LONG_LONG, MPI_SUM, 0, MPI_COMM_WORLD);
    MPI_Reduce(&splits, &sum_split, 1, MPI_LONG_LONG, MPI_SUM, 0, MPI_COMM_WORLD);
    MPI_Reduce(&nodes, &sum_nodes, 1, MPI_LONG_LONG, MPI_SUM, 0, MPI_COMM_WORLD);

    if (rank == 0) {
        std::printf("best_cost=%d\n", global.cost);
        std::printf("best_tour=");
        for (int v : final_path) std::printf(" %d", v);
        std::printf("\nrequests=%lld splits=%lld nodes=%lld\n", sum_req,
                    sum_split, sum_nodes);
        std::printf("energy_total=%.6f\n", total_energy);
    }
    MPI_Finalize();
    return 0;
}
