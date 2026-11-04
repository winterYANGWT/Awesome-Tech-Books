#pragma once
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <new>
#include <vector>

constexpr int NB_N = 800;
constexpr int NB_STEPS = 200;
constexpr double NB_DT = 0.001;
constexpr double NB_SOFTENING = 1e-9;
// 三种实现必须用同一个间距；这里固定 64，避免头文件里的对齐触发
// -Winterference-size（24.3 的示例仍用标准常量）。
constexpr std::size_t NB_LINE = 64;

struct Vec3 {
    double x, y, z;
};

struct Particle {
    Vec3 pos, vel;
    double mass;
};

struct alignas(NB_LINE) ForceRow {
    Vec3 f[NB_N];
};

inline std::vector<Particle> make_particles() {
    std::vector<Particle> p(NB_N);
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

inline double checksum(const std::vector<Particle>& p) {
    double sum = 0;
    for (const Particle& q : p)
        sum += q.mass * (q.pos.x + 2 * q.pos.y + 3 * q.pos.z);
    return sum;
}

inline void force_pair(const std::vector<Particle>& p, int q, int k, Vec3& f) {
    double dx = p[k].pos.x - p[q].pos.x;
    double dy = p[k].pos.y - p[q].pos.y;
    double dz = p[k].pos.z - p[q].pos.z;
    double r2 = dx * dx + dy * dy + dz * dz + NB_SOFTENING;
    double inv = 1.0 / (r2 * std::sqrt(r2));
    f.x = p[q].mass * p[k].mass * inv * dx;
    f.y = p[q].mass * p[k].mass * inv * dy;
    f.z = p[q].mass * p[k].mass * inv * dz;
}
