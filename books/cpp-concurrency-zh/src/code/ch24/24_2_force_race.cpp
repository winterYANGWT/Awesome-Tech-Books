// mode: race
// expect: data race
#include <cmath>
#include <cstdio>
#include <thread>
#include <vector>

constexpr int N = 128;
constexpr int T = 4;
constexpr double SOFTENING = 1e-9;

struct Vec3 {
    double x, y, z;
};

struct Particle {
    Vec3 pos;
    double mass;
};

std::vector<Particle> p(N);
std::vector<Vec3> forces(N);

static void worker(int tid) {
    for (int q = tid; q < N; q += T) {
        for (int k = q + 1; k < N; ++k) {
            double dx = p[k].pos.x - p[q].pos.x;
            double dy = p[k].pos.y - p[q].pos.y;
            double dz = p[k].pos.z - p[q].pos.z;
            double r2 = dx * dx + dy * dy + dz * dz + SOFTENING;
            double inv = 1.0 / (r2 * std::sqrt(r2));
            double fx = p[q].mass * p[k].mass * inv * dx;
            double fy = p[q].mass * p[k].mass * inv * dy;
            double fz = p[q].mass * p[k].mass * inv * dz;
            // 线程只为 q 负责，却写了 forces[k]：这里和另一个线程相遇。
            forces[q].x += fx;
            forces[q].y += fy;
            forces[q].z += fz;
            forces[k].x -= fx;
            forces[k].y -= fy;
            forces[k].z -= fz;
        }
    }
}

int main() {
    for (int i = 0; i < N; ++i) {
        p[i].pos = {i * 0.01, (i % 7) * 0.02, (i % 11) * 0.03};
        p[i].mass = 0.5 + (i % 5) * 0.1;
    }
    std::vector<std::thread> threads;
    for (int t = 0; t < T; ++t) threads.emplace_back(worker, t);
    for (std::thread& th : threads) th.join();

    double sum = 0;
    for (int q = 0; q < N; ++q) sum += forces[q].x + forces[q].y + forces[q].z;
    std::printf("done checksum=%.6g\n", sum);
    return 0;
}
