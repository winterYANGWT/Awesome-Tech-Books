// md_common.h: Lennard-Jones 分子动力学的 CPU 部分，第 27–29 章共用。
// 约化单位：粒子质量、ε、σ 都是 1。周期性立方盒子，粒子初始排成 FCC 晶格。
#pragma once
#include <chrono>
#include <cmath>
#include <cstdio>
#include <random>
#include <vector>

struct Params {
    int nc = 24;               // 每个方向的 FCC 晶胞数，粒子数 = 4·nc³
    double rho = 0.8442;       // 数密度
    double temp = 0.722;       // 初始温度
    double dt = 0.005;         // 时间步长
    double rc = 2.5;           // 势能截断半径
    double skin = 0.3;         // 邻居表多留的一层，邻居表半径 = rc + skin
    int steps = 500;
    int rebuild_every = 10;    // 每隔几步重建邻居表
    int report_every = 100;    // 每隔几步报告一次能量
};

// ANCHOR: system
// 每个物理量一个数组（第 8 章的 SoA 布局）。
struct System {
    int n = 0;                          // 粒子数
    double box = 0;                     // 盒子边长
    std::vector<double> x, y, z;        // 位置
    std::vector<double> vx, vy, vz;     // 速度
    std::vector<double> fx, fy, fz;     // 受力
    std::vector<double> epot;           // 每个粒子分到的势能
    int max_nbr = 0;                    // 每个粒子最多记录的邻居数
    std::vector<int> nbr_count;         // 粒子 i 的邻居个数
    std::vector<int> nbr;               // 粒子 i 的第 k 个邻居：nbr[i * max_nbr + k]
};
// ANCHOR_END: system

inline double nearest_image(double d, double box) { return d - box * std::round(d / box); }

// 粒子放在 FCC 晶格上，速度取正态分布、去掉整体平动，再缩放到指定温度。
inline void init_system(System& s, const Params& p) {
    s.n = 4 * p.nc * p.nc * p.nc;
    s.box = std::cbrt(s.n / p.rho);
    for (auto* v : {&s.x, &s.y, &s.z, &s.vx, &s.vy, &s.vz, &s.fx, &s.fy, &s.fz, &s.epot}) v->assign(s.n, 0.0);
    const double a = s.box / p.nc;
    const double basis[4][3] = {{0, 0, 0}, {0.5, 0.5, 0}, {0.5, 0, 0.5}, {0, 0.5, 0.5}};
    int i = 0;
    for (int cx = 0; cx < p.nc; ++cx)
        for (int cy = 0; cy < p.nc; ++cy)
            for (int cz = 0; cz < p.nc; ++cz)
                for (auto& b : basis) {
                    s.x[i] = (cx + b[0] + 0.25) * a;
                    s.y[i] = (cy + b[1] + 0.25) * a;
                    s.z[i] = (cz + b[2] + 0.25) * a;
                    ++i;
                }
    std::mt19937 rng(12345);
    std::normal_distribution<double> g(0.0, 1.0);
    double px = 0, py = 0, pz = 0;
    for (int k = 0; k < s.n; ++k) {
        s.vx[k] = g(rng), s.vy[k] = g(rng), s.vz[k] = g(rng);
        px += s.vx[k], py += s.vy[k], pz += s.vz[k];
    }
    double ke = 0;
    for (int k = 0; k < s.n; ++k) {
        s.vx[k] -= px / s.n, s.vy[k] -= py / s.n, s.vz[k] -= pz / s.n;
        ke += 0.5 * (s.vx[k] * s.vx[k] + s.vy[k] * s.vy[k] + s.vz[k] * s.vz[k]);
    }
    double scale = std::sqrt(1.5 * s.n * p.temp / ke);    // 动能 = 3/2·N·T
    for (int k = 0; k < s.n; ++k) s.vx[k] *= scale, s.vy[k] *= scale, s.vz[k] *= scale;
}

// ANCHOR: neighbors
// 用格子法建邻居表：把盒子切成边长不小于 rlist 的格子，每个粒子只需要查自己和周围 26 个格子。
// half 为真时只记录编号比自己大的邻居（每对只记一次，给利用牛顿第三定律的求力函数用）。
inline void build_neighbors(System& s, double rlist, bool half) {
    const int nc = int(s.box / rlist);                    // 每个方向的格子数
    const double cell = s.box / nc;
    std::vector<int> head(nc * nc * nc, -1), next(s.n);   // 每个格子里的粒子串成链表
    auto cell_of = [&](double v) { int c = int(v / cell); return c < 0 ? 0 : c >= nc ? nc - 1 : c; };
    for (int i = 0; i < s.n; ++i) {
        int c = (cell_of(s.x[i]) * nc + cell_of(s.y[i])) * nc + cell_of(s.z[i]);
        next[i] = head[c];
        head[c] = i;
    }
    s.nbr_count.assign(s.n, 0);
    s.nbr.assign(size_t(s.n) * s.max_nbr, 0);
    const double r2max = rlist * rlist;
    for (int i = 0; i < s.n; ++i) {
        int ci = cell_of(s.x[i]), cj = cell_of(s.y[i]), ck = cell_of(s.z[i]);
        for (int dx = -1; dx <= 1; ++dx)
            for (int dy = -1; dy <= 1; ++dy)
                for (int dz = -1; dz <= 1; ++dz) {
                    int c = (((ci + dx + nc) % nc) * nc + (cj + dy + nc) % nc) * nc + (ck + dz + nc) % nc;
                    for (int j = head[c]; j >= 0; j = next[j]) {
                        if (j == i || (half && j < i)) continue;
                        double rx = nearest_image(s.x[i] - s.x[j], s.box);
                        double ry = nearest_image(s.y[i] - s.y[j], s.box);
                        double rz = nearest_image(s.z[i] - s.z[j], s.box);
                        if (rx * rx + ry * ry + rz * rz < r2max && s.nbr_count[i] < s.max_nbr)
                            s.nbr[size_t(i) * s.max_nbr + s.nbr_count[i]++] = j;
                    }
                }
    }
}
// ANCHOR_END: neighbors

// ANCHOR: forces
// Lennard-Jones 力。邻居表只记了 j > i 的一半，每对算一次，用牛顿第三定律同时更新两个粒子。
inline void forces_cpu(System& s, double rc) {
    const double rc2 = rc * rc, inv_rc6 = 1.0 / (rc2 * rc2 * rc2);
    const double e_shift = 4.0 * inv_rc6 * (inv_rc6 - 1.0);   // 平移势能，使其在截断处为 0
    std::fill(s.fx.begin(), s.fx.end(), 0.0);
    std::fill(s.fy.begin(), s.fy.end(), 0.0);
    std::fill(s.fz.begin(), s.fz.end(), 0.0);
    std::fill(s.epot.begin(), s.epot.end(), 0.0);
    for (int i = 0; i < s.n; ++i) {
        for (int k = 0; k < s.nbr_count[i]; ++k) {
            int j = s.nbr[size_t(i) * s.max_nbr + k];
            double dx = nearest_image(s.x[i] - s.x[j], s.box);
            double dy = nearest_image(s.y[i] - s.y[j], s.box);
            double dz = nearest_image(s.z[i] - s.z[j], s.box);
            double r2 = dx * dx + dy * dy + dz * dz;
            if (r2 >= rc2) continue;
            double inv_r2 = 1.0 / r2, inv_r6 = inv_r2 * inv_r2 * inv_r2;
            double f = 24.0 * inv_r2 * inv_r6 * (2.0 * inv_r6 - 1.0);   // 力的大小除以 r
            double e = 4.0 * inv_r6 * (inv_r6 - 1.0) - e_shift;
            s.fx[i] += f * dx, s.fy[i] += f * dy, s.fz[i] += f * dz;
            s.fx[j] -= f * dx, s.fy[j] -= f * dy, s.fz[j] -= f * dz;
            s.epot[i] += 0.5 * e, s.epot[j] += 0.5 * e;
        }
    }
}
// ANCHOR_END: forces

// ANCHOR: integrate
// 速度 Verlet 的两半：kick 用力更新半步速度，drift 用速度更新位置（并把粒子折回盒子里）。
inline void kick(System& s, double half_dt) {
    for (int i = 0; i < s.n; ++i) s.vx[i] += half_dt * s.fx[i], s.vy[i] += half_dt * s.fy[i], s.vz[i] += half_dt * s.fz[i];
}
inline void drift(System& s, double dt) {
    for (int i = 0; i < s.n; ++i) {
        s.x[i] += dt * s.vx[i], s.y[i] += dt * s.vy[i], s.z[i] += dt * s.vz[i];
        s.x[i] -= s.box * std::floor(s.x[i] / s.box);
        s.y[i] -= s.box * std::floor(s.y[i] / s.box);
        s.z[i] -= s.box * std::floor(s.z[i] / s.box);
    }
}
// ANCHOR_END: integrate

inline double kinetic(const System& s) {
    double ke = 0;
    for (int i = 0; i < s.n; ++i) ke += 0.5 * (s.vx[i] * s.vx[i] + s.vy[i] * s.vy[i] + s.vz[i] * s.vz[i]);
    return ke;
}
inline double potential(const System& s) {
    double pe = 0;
    for (double e : s.epot) pe += e;
    return pe;
}

// 分阶段累计时间。
struct Timer {
    double total = 0;
    std::chrono::steady_clock::time_point t0;
    void start() { t0 = std::chrono::steady_clock::now(); }
    void stop() { total += std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - t0).count(); }
};
