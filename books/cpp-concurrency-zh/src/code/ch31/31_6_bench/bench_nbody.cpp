// 基准：n=800 的 Allgather n 体，时间只含 100 个时间步
#include <mpi.h>

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <random>
#include <vector>

int main(int argc, char** argv) {
    MPI_Init(&argc, &argv);
    int rank = 0, p = 0;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &p);

    constexpr int n = 800, steps = 100;
    constexpr double dt = 0.0005, eps2 = 0.01;
    const int loc_n = n / p;

    std::vector<double> x(n), y(n), vx(n), vy(n);
    if (rank == 0) {
        std::mt19937 rng(2026);
        std::uniform_real_distribution<double> pos(-1.0, 1.0);
        std::uniform_real_distribution<double> vel(-0.2, 0.2);
        for (int i = 0; i < n; ++i) {
            x[i] = pos(rng);
            y[i] = pos(rng);
            vx[i] = vel(rng);
            vy[i] = vel(rng);
        }
    }
    std::vector<double> lx(loc_n), ly(loc_n), lvx(loc_n), lvy(loc_n);
    MPI_Scatter(x.data(), loc_n, MPI_DOUBLE, lx.data(), loc_n, MPI_DOUBLE, 0,
                MPI_COMM_WORLD);
    MPI_Scatter(y.data(), loc_n, MPI_DOUBLE, ly.data(), loc_n, MPI_DOUBLE, 0,
                MPI_COMM_WORLD);
    MPI_Scatter(vx.data(), loc_n, MPI_DOUBLE, lvx.data(), loc_n, MPI_DOUBLE, 0,
                MPI_COMM_WORLD);
    MPI_Scatter(vy.data(), loc_n, MPI_DOUBLE, lvy.data(), loc_n, MPI_DOUBLE, 0,
                MPI_COMM_WORLD);

    std::vector<double> gx(n), gy(n), fx(loc_n), fy(loc_n);
    MPI_Barrier(MPI_COMM_WORLD);
    const double t0 = MPI_Wtime();
    for (int step = 0; step < steps; ++step) {
        MPI_Allgather(lx.data(), loc_n, MPI_DOUBLE, gx.data(), loc_n, MPI_DOUBLE,
                      MPI_COMM_WORLD);
        MPI_Allgather(ly.data(), loc_n, MPI_DOUBLE, gy.data(), loc_n, MPI_DOUBLE,
                      MPI_COMM_WORLD);
        std::fill(fx.begin(), fx.end(), 0.0);
        std::fill(fy.begin(), fy.end(), 0.0);
        const int base = rank * loc_n;
        for (int i = 0; i < loc_n; ++i) {
            for (int j = 0; j < n; ++j) {
                const double dx = gx[j] - gx[base + i];
                const double dy = gy[j] - gy[base + i];
                const double r2 = dx * dx + dy * dy + eps2;
                const double invr3 = 1.0 / (r2 * std::sqrt(r2));
                fx[i] += dx * invr3;
                fy[i] += dy * invr3;
            }
        }
        for (int i = 0; i < loc_n; ++i) {
            lvx[i] += dt * fx[i];
            lvy[i] += dt * fy[i];
            lx[i] += dt * lvx[i];
            ly[i] += dt * lvy[i];
        }
    }
    const double t1 = MPI_Wtime();

    MPI_Gather(lx.data(), loc_n, MPI_DOUBLE, x.data(), loc_n, MPI_DOUBLE, 0,
               MPI_COMM_WORLD);
    MPI_Gather(ly.data(), loc_n, MPI_DOUBLE, y.data(), loc_n, MPI_DOUBLE, 0,
               MPI_COMM_WORLD);
    MPI_Gather(lvx.data(), loc_n, MPI_DOUBLE, vx.data(), loc_n, MPI_DOUBLE, 0,
               MPI_COMM_WORLD);
    MPI_Gather(lvy.data(), loc_n, MPI_DOUBLE, vy.data(), loc_n, MPI_DOUBLE, 0,
               MPI_COMM_WORLD);

    double local_t = t1 - t0, max_t = 0.0;
    MPI_Reduce(&local_t, &max_t, 1, MPI_DOUBLE, MPI_MAX, 0, MPI_COMM_WORLD);
    if (rank == 0) {
        double sum = 0.0;
        for (int i = 0; i < n; ++i)
            sum += x[i] + 2.0 * y[i] + 3.0 * vx[i] + 5.0 * vy[i];
        std::printf("nbody procs=%d time_ms=%.3f checksum=%.9f\n", p,
                    1000.0 * max_t, sum);
    }
    MPI_Finalize();
    return 0;
}
