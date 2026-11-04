#include "nbody_common.hpp"
#include "tsp_check.hpp"

#include <algorithm>
#include <barrier>
#include <chrono>
#include <cstdio>
#include <thread>
#include <vector>

static double run_nbody(int threads) {
    std::vector<Particle> p = make_particles();
    std::vector<Vec3> forces(NB_N);
    std::vector<ForceRow> rows(threads);
    std::barrier<> bar(threads);

    std::vector<std::thread> pool;
    for (int id = 0; id < threads; ++id) {
        pool.emplace_back([&, id] {
            for (int step = 0; step < NB_STEPS; ++step) {
                ForceRow& row = rows[id];
                for (int q = 0; q < NB_N; ++q) row.f[q] = {0, 0, 0};

                int q0 = static_cast<int>(static_cast<long>(NB_N) * id / threads);
                int q1 = static_cast<int>(static_cast<long>(NB_N) * (id + 1) / threads);
                for (int q = q0; q < q1; ++q) {
                    for (int k = q + 1; k < NB_N; ++k) {
                        Vec3 f;
                        force_pair(p, q, k, f);
                        row.f[q].x += f.x;
                        row.f[q].y += f.y;
                        row.f[q].z += f.z;
                        row.f[k].x -= f.x;
                        row.f[k].y -= f.y;
                        row.f[k].z -= f.z;
                    }
                }
                bar.arrive_and_wait();

                for (int q = q0; q < q1; ++q) {
                    Vec3 a{0, 0, 0};
                    for (int r = 0; r < threads; ++r) {
                        a.x += rows[r].f[q].x;
                        a.y += rows[r].f[q].y;
                        a.z += rows[r].f[q].z;
                    }
                    forces[q] = a;
                    p[q].vel.x += a.x / p[q].mass * NB_DT;
                    p[q].vel.y += a.y / p[q].mass * NB_DT;
                    p[q].vel.z += a.z / p[q].mass * NB_DT;
                    p[q].pos.x += p[q].vel.x * NB_DT;
                    p[q].pos.y += p[q].vel.y * NB_DT;
                    p[q].pos.z += p[q].vel.z * NB_DT;
                }
                bar.arrive_and_wait();
            }
        });
    }
    for (std::thread& th : pool) th.join();
    return checksum(p);
}

int main() {
    std::fprintf(stderr, "stdthread tsp_best=%d\n", tspcheck::best_cost_of_graph());
    std::printf("threads,ms\n");
    for (int threads : {1, 2, 4, 8, 16}) {
        std::vector<double> times;
        double sum = 0;
        for (int rep = 0; rep < 3; ++rep) {
            auto t0 = std::chrono::steady_clock::now();
            sum = run_nbody(threads);
            auto t1 = std::chrono::steady_clock::now();
            times.push_back(std::chrono::duration<double, std::milli>(t1 - t0).count());
        }
        std::sort(times.begin(), times.end());
        std::fprintf(stderr, "stdthread threads=%d checksum=%.17g\n", threads, sum);
        std::printf("%d,%.3f\n", threads, times[1]);
    }
    return 0;
}
