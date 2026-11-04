#include "nbody_common.hpp"
#include "tsp_check.hpp"

#include <algorithm>
#include <chrono>
#include <cstdio>
#include <vector>

#include <omp.h>

static double run_nbody(int threads) {
    std::vector<Particle> p = make_particles();
    std::vector<Vec3> forces(NB_N);
    std::vector<ForceRow> rows(threads);

#pragma omp parallel num_threads(threads)
    {
        int tid = omp_get_thread_num();
        for (int step = 0; step < NB_STEPS; ++step) {
            for (int q = 0; q < NB_N; ++q) rows[tid].f[q] = {0, 0, 0};

#pragma omp for schedule(static)
            for (int q = 0; q < NB_N; ++q) {
                for (int k = q + 1; k < NB_N; ++k) {
                    Vec3 f;
                    force_pair(p, q, k, f);
                    rows[tid].f[q].x += f.x;
                    rows[tid].f[q].y += f.y;
                    rows[tid].f[q].z += f.z;
                    rows[tid].f[k].x -= f.x;
                    rows[tid].f[k].y -= f.y;
                    rows[tid].f[k].z -= f.z;
                }
            }

#pragma omp for schedule(static)
            for (int q = 0; q < NB_N; ++q) {
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
        }
    }
    return checksum(p);
}

int main() {
    std::fprintf(stderr, "openmp tsp_best=%d\n", tspcheck::best_cost_of_graph());
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
        std::fprintf(stderr, "openmp threads=%d checksum=%.17g\n", threads, sum);
        std::printf("%d,%.3f\n", threads, times[1]);
    }
    return 0;
}
