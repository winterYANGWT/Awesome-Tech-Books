// 基本 n 体：每步 MPI_Allgather 所有位置，各进程算自己那一块粒子的力
#include <mpi.h>

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <random>
#include <vector>

namespace {
constexpr int N = 400;
constexpr int STEPS = 200;
constexpr double DT = 0.0005;
constexpr double EPS2 = 0.01;
}  // namespace

int main(int argc, char** argv) {
    MPI_Init(&argc, &argv);
    int rank = 0, p = 0;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &p);
    if (N % p != 0) {
        if (rank == 0) std::printf("N=%d 不能被 p=%d 整除\n", N, p);
        MPI_Finalize();
        return 1;
    }
    const int loc_n = N / p;

    std::vector<double> x(N), y(N), vx(N), vy(N), mass(N);
    if (rank == 0) {
        std::mt19937 rng(2026);
        std::uniform_real_distribution<double> pos(-1.0, 1.0);
        std::uniform_real_distribution<double> vel(-0.2, 0.2);
        for (int i = 0; i < N; ++i) {
            x[i] = pos(rng);
            y[i] = pos(rng);
            vx[i] = vel(rng);
            vy[i] = vel(rng);
            mass[i] = 1.0;  // 等质量：f_qk = -f_kq 对加速度也成立
        }
    }
    MPI_Bcast(mass.data(), N, MPI_DOUBLE, 0, MPI_COMM_WORLD);

    std::vector<double> lx(loc_n), ly(loc_n), lvx(loc_n), lvy(loc_n);
    MPI_Scatter(x.data(), loc_n, MPI_DOUBLE, lx.data(), loc_n, MPI_DOUBLE, 0,
                MPI_COMM_WORLD);
    MPI_Scatter(y.data(), loc_n, MPI_DOUBLE, ly.data(), loc_n, MPI_DOUBLE, 0,
                MPI_COMM_WORLD);
    MPI_Scatter(vx.data(), loc_n, MPI_DOUBLE, lvx.data(), loc_n, MPI_DOUBLE, 0,
                MPI_COMM_WORLD);
    MPI_Scatter(vy.data(), loc_n, MPI_DOUBLE, lvy.data(), loc_n, MPI_DOUBLE, 0,
                MPI_COMM_WORLD);

    std::vector<double> gx(N), gy(N);
    std::vector<double> fx(loc_n, 0.0), fy(loc_n, 0.0);

    MPI_Barrier(MPI_COMM_WORLD);
    const double t0 = MPI_Wtime();
    // ANCHOR: step
    for (int step = 0; step < STEPS; ++step) {
        MPI_Allgather(lx.data(), loc_n, MPI_DOUBLE, gx.data(), loc_n, MPI_DOUBLE,
                      MPI_COMM_WORLD);
        MPI_Allgather(ly.data(), loc_n, MPI_DOUBLE, gy.data(), loc_n, MPI_DOUBLE,
                      MPI_COMM_WORLD);

        std::fill(fx.begin(), fx.end(), 0.0);
        std::fill(fy.begin(), fy.end(), 0.0);
        const int base = rank * loc_n;
        for (int i = 0; i < loc_n; ++i) {
            double ax = 0.0, ay = 0.0;
            for (int j = 0; j < N; ++j) {
                const double dx = gx[j] - gx[base + i];
                const double dy = gy[j] - gy[base + i];
                const double r2 = dx * dx + dy * dy + EPS2;
                const double invr3 = 1.0 / (r2 * std::sqrt(r2));
                ax += mass[j] * dx * invr3;
                ay += mass[j] * dy * invr3;
            }
            fx[i] = ax;
            fy[i] = ay;
        }
        for (int i = 0; i < loc_n; ++i) {
            lvx[i] += DT * fx[i];
            lvy[i] += DT * fy[i];
            lx[i] += DT * lvx[i];
            ly[i] += DT * lvy[i];
        }
    }
    const double t1 = MPI_Wtime();
    // ANCHOR_END: step

    // 按全局下标顺序汇总，校验和与 p 无关
    MPI_Gather(lx.data(), loc_n, MPI_DOUBLE, gx.data(), loc_n, MPI_DOUBLE, 0,
               MPI_COMM_WORLD);
    MPI_Gather(ly.data(), loc_n, MPI_DOUBLE, gy.data(), loc_n, MPI_DOUBLE, 0,
               MPI_COMM_WORLD);
    MPI_Gather(lvx.data(), loc_n, MPI_DOUBLE, x.data(), loc_n, MPI_DOUBLE, 0,
               MPI_COMM_WORLD);
    MPI_Gather(lvy.data(), loc_n, MPI_DOUBLE, y.data(), loc_n, MPI_DOUBLE, 0,
               MPI_COMM_WORLD);

    double local_t = t1 - t0, max_t = 0.0;
    MPI_Reduce(&local_t, &max_t, 1, MPI_DOUBLE, MPI_MAX, 0, MPI_COMM_WORLD);

    if (rank == 0) {
        double checksum = 0.0;
        for (int i = 0; i < N; ++i)
            checksum += gx[i] + 2.0 * gy[i] + 3.0 * x[i] + 5.0 * y[i];
        std::printf("np=%d checksum=%.12f ms=%.3f\n", p, checksum,
                    1000.0 * max_t);
    }
    MPI_Finalize();
    return 0;
}
