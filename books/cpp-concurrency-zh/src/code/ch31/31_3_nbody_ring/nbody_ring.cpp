// 简化 n 体：位置块沿环传递，利用 f_qk = -f_kq 只算一半相互作用
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

    const int next = (rank + 1) % p;
    const int prev = (rank - 1 + p) % p;
    std::vector<double> fx(loc_n, 0.0), fy(loc_n, 0.0);
    std::vector<double> tx(loc_n), ty(loc_n), tfx(loc_n), tfy(loc_n);
    std::vector<double> rx(loc_n), ry(loc_n), rfx(loc_n), rfy(loc_n);

    MPI_Barrier(MPI_COMM_WORLD);
    const double t0 = MPI_Wtime();
    for (int step = 0; step < STEPS; ++step) {
        std::fill(fx.begin(), fx.end(), 0.0);
        std::fill(fy.begin(), fy.end(), 0.0);
        tx = lx;
        ty = ly;
        std::fill(tfx.begin(), tfx.end(), 0.0);
        std::fill(tfy.begin(), tfy.end(), 0.0);

        // ANCHOR: ring
        for (int phase = 0; phase < p; ++phase) {
            MPI_Sendrecv(tx.data(), loc_n, MPI_DOUBLE, next, 0, rx.data(), loc_n,
                         MPI_DOUBLE, prev, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            MPI_Sendrecv(ty.data(), loc_n, MPI_DOUBLE, next, 1, ry.data(), loc_n,
                         MPI_DOUBLE, prev, 1, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            MPI_Sendrecv(tfx.data(), loc_n, MPI_DOUBLE, next, 2, rfx.data(),
                         loc_n, MPI_DOUBLE, prev, 2, MPI_COMM_WORLD,
                         MPI_STATUS_IGNORE);
            MPI_Sendrecv(tfy.data(), loc_n, MPI_DOUBLE, next, 3, rfy.data(),
                         loc_n, MPI_DOUBLE, prev, 3, MPI_COMM_WORLD,
                         MPI_STATUS_IGNORE);

            const int src = (rank - 1 - phase + 4 * p) % p;
            const int base_i = rank * loc_n;
            const int base_j = src * loc_n;
            for (int i = 0; i < loc_n; ++i) {
                const int gi = base_i + i;
                for (int j = 0; j < loc_n; ++j) {
                    const int gj = base_j + j;
                    if (gi >= gj) continue;  // 每一对只算一次
                    const double dx = rx[j] - lx[i];
                    const double dy = ry[j] - ly[i];
                    const double r2 = dx * dx + dy * dy + EPS2;
                    const double f = mass[gj] / (r2 * std::sqrt(r2));
                    fx[i] += f * dx;
                    fy[i] += f * dy;
                    rfx[j] -= f * dx;
                    rfy[j] -= f * dy;
                }
            }
            tx = rx;
            ty = ry;
            tfx = rfx;
            tfy = rfy;
        }
        // ANCHOR_END: ring
        // 自己的块转完一圈，带回其他进程记下的反向力
        for (int i = 0; i < loc_n; ++i) {
            fx[i] += tfx[i];
            fy[i] += tfy[i];
        }
        for (int i = 0; i < loc_n; ++i) {
            lvx[i] += DT * fx[i];
            lvy[i] += DT * fy[i];
            lx[i] += DT * lvx[i];
            ly[i] += DT * lvy[i];
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
        double checksum = 0.0;
        for (int i = 0; i < N; ++i)
            checksum += x[i] + 2.0 * y[i] + 3.0 * vx[i] + 5.0 * vy[i];
        std::printf("np=%d checksum=%.12f ms=%.3f\n", p, checksum,
                    1000.0 * max_t);
    }
    MPI_Finalize();
    return 0;
}
