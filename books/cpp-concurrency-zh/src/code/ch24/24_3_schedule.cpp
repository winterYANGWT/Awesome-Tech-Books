// mode: run
// flags: -fopenmp
// tsan: no（libgomp 未插桩）
// output: varies
// expect: threads,mode,ms,checksum
#include <chrono>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <vector>

#include <omp.h>

constexpr int N = 800;
constexpr int STEPS = 200;
constexpr double DT = 0.001;
constexpr double SOFTENING = 1e-9;
constexpr std::size_t kLine = std::hardware_destructive_interference_size;

struct Vec3 {
    double x, y, z;
};

struct Particle {
    Vec3 pos, vel;
    double mass;
};

struct alignas(kLine) ForceRow {
    Vec3 f[N];
};

enum Mode { BLOCK = 0, FORCE_CYCLIC = 1, ALL_CYCLIC = 2 };

static std::vector<Particle> make_particles() {
    std::vector<Particle> p(N);
    std::uint64_t s = 12345;
    auto rnd = [&s]() {
        s = s * 6364136223846793005ULL + 1442695040888963407ULL;
        return static_cast<double>(s >> 11) / 9007199254740992.0;
    };
    for (Particle& q : p) {
        q.pos = {rnd() * 2 - 1, rnd() * 2 - 1, rnd() * 2 - 1};
        q.vel = {rnd() * 0.1, rnd() * 0.1, rnd() * 0.1};
        q.mass = 0.5 + rnd();
    }
    return p;
}

static double checksum(const std::vector<Particle>& p) {
    double sum = 0;
    for (const Particle& q : p)
        sum += q.mass * (q.pos.x + 2 * q.pos.y + 3 * q.pos.z);
    return sum;
}

static void force_pair(const std::vector<Particle>& p, int q, int k, Vec3& f) {
    double dx = p[k].pos.x - p[q].pos.x;
    double dy = p[k].pos.y - p[q].pos.y;
    double dz = p[k].pos.z - p[q].pos.z;
    double r2 = dx * dx + dy * dy + dz * dz + SOFTENING;
    double inv = 1.0 / (r2 * std::sqrt(r2));
    f.x = p[q].mass * p[k].mass * inv * dx;
    f.y = p[q].mass * p[k].mass * inv * dy;
    f.z = p[q].mass * p[k].mass * inv * dz;
}

static double simulate(int threads, Mode mode) {
    std::vector<Particle> p = make_particles();
    std::vector<Vec3> forces(N);
    std::vector<ForceRow> rows(threads);
#pragma omp parallel num_threads(threads)
    {
        int tid = omp_get_thread_num();
        for (int step = 0; step < STEPS; ++step) {
            for (int q = 0; q < N; ++q) rows[tid].f[q] = {0, 0, 0};

            if (mode == BLOCK) {
#pragma omp for schedule(static)
                for (int q = 0; q < N; ++q) {
                    for (int k = q + 1; k < N; ++k) {
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
            } else {
#pragma omp for schedule(static, 1)
                for (int q = 0; q < N; ++q) {
                    for (int k = q + 1; k < N; ++k) {
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
            }

            if (mode == ALL_CYCLIC) {
#pragma omp for schedule(static, 1)
                for (int q = 0; q < N; ++q) {
                    Vec3 a{0, 0, 0};
                    for (int r = 0; r < threads; ++r) {
                        a.x += rows[r].f[q].x;
                        a.y += rows[r].f[q].y;
                        a.z += rows[r].f[q].z;
                    }
                    forces[q] = a;
                }
            } else {
#pragma omp for schedule(static)
                for (int q = 0; q < N; ++q) {
                    Vec3 a{0, 0, 0};
                    for (int r = 0; r < threads; ++r) {
                        a.x += rows[r].f[q].x;
                        a.y += rows[r].f[q].y;
                        a.z += rows[r].f[q].z;
                    }
                    forces[q] = a;
                }
            }

            if (mode == ALL_CYCLIC) {
#pragma omp for schedule(static, 1)
                for (int q = 0; q < N; ++q) {
                    p[q].vel.x += forces[q].x / p[q].mass * DT;
                    p[q].vel.y += forces[q].y / p[q].mass * DT;
                    p[q].vel.z += forces[q].z / p[q].mass * DT;
                    p[q].pos.x += p[q].vel.x * DT;
                    p[q].pos.y += p[q].vel.y * DT;
                    p[q].pos.z += p[q].vel.z * DT;
                }
            } else {
#pragma omp for schedule(static)
                for (int q = 0; q < N; ++q) {
                    p[q].vel.x += forces[q].x / p[q].mass * DT;
                    p[q].vel.y += forces[q].y / p[q].mass * DT;
                    p[q].vel.z += forces[q].z / p[q].mass * DT;
                    p[q].pos.x += p[q].vel.x * DT;
                    p[q].pos.y += p[q].vel.y * DT;
                    p[q].pos.z += p[q].vel.z * DT;
                }
            }
        }
    }
    return checksum(p);
}

int main() {
    simulate(1, BLOCK);   // 预热

    std::printf("threads,mode,ms,checksum\n");
    for (int threads : {1, 2, 4, 8}) {
        for (Mode mode : {BLOCK, FORCE_CYCLIC, ALL_CYCLIC}) {
            auto t0 = std::chrono::steady_clock::now();
            double sum = simulate(threads, mode);
            auto t1 = std::chrono::steady_clock::now();
            double ms = std::chrono::duration<double, std::milli>(t1 - t0).count();
            std::printf("%d,%s,%.3f,%.17g\n", threads,
                        mode == BLOCK ? "block"
                        : mode == FORCE_CYCLIC ? "force_cyclic"
                                               : "all_cyclic",
                        ms, sum);
        }
    }
    return 0;
}
