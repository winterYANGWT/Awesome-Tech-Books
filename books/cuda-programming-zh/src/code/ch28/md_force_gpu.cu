// md_force_gpu.cu: 只把求力搬到 GPU，其余（邻居表、积分）仍在 CPU 上。
// 每步把位置拷上去、算力、把力拷回来。求力 kernel 有全 double 和混合精度两个版本。
//   nvcc -O2 -arch=native md_force_gpu.cu -o md_force_gpu && ./md_force_gpu [double|mixed] [晶胞数] [步数]
#include "../ch27/md_common.h"
#include <cstdlib>
#include <cstring>

#define CHECK(call)                                                        \
    do {                                                                   \
        cudaError_t err_ = (call);                                         \
        if (err_ != cudaSuccess) {                                         \
            fprintf(stderr, "%s:%d: %s (%s)\n", __FILE__, __LINE__,        \
                    cudaGetErrorName(err_), cudaGetErrorString(err_));     \
            exit(1);                                                       \
        }                                                                  \
    } while (0)

// ANCHOR: kernel
// 一个线程负责一个粒子 i，遍历它的全部邻居，只写自己的力和势能，所以不会和别的线程冲突。
// 邻居表转置存放：粒子 i 的第 k 个邻居在 nbr[k * n + i]，相邻线程读相邻的地址。
// Real 决定计算用什么精度。两个坐标相减总是用 double，避免两个大数相减损失精度；
// 相减之后的位移不超过盒长，之后的最小镜像和 LJ 计算都用 Real。
template <class Real>
__global__ void forces_gpu(int n, Real box, Real inv_box, Real rc2, Real e_shift, const int* __restrict__ nbr_count,
                           const int* __restrict__ nbr, const double* __restrict__ x,
                           const double* __restrict__ y, const double* __restrict__ z,
                           double* fx, double* fy, double* fz, double* epot) {
    int i = blockIdx.x * blockDim.x + threadIdx.x;
    if (i >= n) return;
    double xi = x[i], yi = y[i], zi = z[i];
    Real ax = 0, ay = 0, az = 0, e = 0;
    for (int k = 0; k < nbr_count[i]; ++k) {
        int j = nbr[k * n + i];
        Real dx = Real(xi - x[j]), dy = Real(yi - y[j]), dz = Real(zi - z[j]);
        dx -= box * rint(dx * inv_box);                // 最小镜像
        dy -= box * rint(dy * inv_box);
        dz -= box * rint(dz * inv_box);
        Real r2 = dx * dx + dy * dy + dz * dz;
        if (r2 >= rc2) continue;
        Real inv_r2 = Real(1) / r2, inv_r6 = inv_r2 * inv_r2 * inv_r2;
        Real f = Real(24) * inv_r2 * inv_r6 * (Real(2) * inv_r6 - Real(1));
        ax += f * dx, ay += f * dy, az += f * dz;
        e += Real(0.5) * (Real(4) * inv_r6 * (inv_r6 - Real(1)) - e_shift);   // 每对的势能两边各记一半
    }
    fx[i] = ax, fy[i] = ay, fz[i] = az, epot[i] = e;
}
// ANCHOR_END: kernel

int main(int argc, char** argv) {
    bool mixed = argc > 1 && !strcmp(argv[1], "mixed");
    Params p;
    if (argc > 2) p.nc = atoi(argv[2]);
    if (argc > 3) p.steps = atoi(argv[3]);
    System s;
    init_system(s, p);
    s.max_nbr = 128;
    const int n = s.n;
    const double rlist = p.rc + p.skin, rc2 = p.rc * p.rc;
    const double inv_rc6 = 1.0 / (rc2 * rc2 * rc2), e_shift = 4.0 * inv_rc6 * (inv_rc6 - 1.0);
    printf("%d 个粒子，%d 步，求力用 %s\n", n, p.steps, mixed ? "混合精度" : "double");

    double *d_x, *d_y, *d_z, *d_fx, *d_fy, *d_fz, *d_e;
    int *d_cnt, *d_nbr;
    for (double** a : {&d_x, &d_y, &d_z, &d_fx, &d_fy, &d_fz, &d_e}) CHECK(cudaMalloc(a, n * sizeof(double)));
    CHECK(cudaMalloc(&d_cnt, n * sizeof(int)));
    CHECK(cudaMalloc(&d_nbr, size_t(n) * s.max_nbr * sizeof(int)));
    std::vector<int> nbr_t(size_t(n) * s.max_nbr);

    Timer t_nbr, t_up, t_kernel, t_down, t_int;
    // 建完整的邻居表（双向），转置后拷到 GPU。
    auto rebuild = [&] {
        t_nbr.start();
        build_neighbors(s, rlist, false);
        for (int i = 0; i < n; ++i)
            for (int k = 0; k < s.nbr_count[i]; ++k) nbr_t[size_t(k) * n + i] = s.nbr[size_t(i) * s.max_nbr + k];
        CHECK(cudaMemcpy(d_cnt, s.nbr_count.data(), n * sizeof(int), cudaMemcpyHostToDevice));
        CHECK(cudaMemcpy(d_nbr, nbr_t.data(), nbr_t.size() * sizeof(int), cudaMemcpyHostToDevice));
        t_nbr.stop();
    };
    // ANCHOR: step
    auto compute_forces = [&] {
        t_up.start();                                 // 位置拷上去
        CHECK(cudaMemcpy(d_x, s.x.data(), n * sizeof(double), cudaMemcpyHostToDevice));
        CHECK(cudaMemcpy(d_y, s.y.data(), n * sizeof(double), cudaMemcpyHostToDevice));
        CHECK(cudaMemcpy(d_z, s.z.data(), n * sizeof(double), cudaMemcpyHostToDevice));
        t_up.stop();
        t_kernel.start();
        if (mixed)
            forces_gpu<float><<<(n + 127) / 128, 128>>>(n, float(s.box), float(1 / s.box), float(rc2),
                                                       float(e_shift), d_cnt, d_nbr,
                                                       d_x, d_y, d_z, d_fx, d_fy, d_fz, d_e);
        else
            forces_gpu<double><<<(n + 127) / 128, 128>>>(n, s.box, 1 / s.box, rc2, e_shift, d_cnt, d_nbr,
                                                        d_x, d_y, d_z, d_fx, d_fy, d_fz, d_e);
        CHECK(cudaGetLastError());
        CHECK(cudaDeviceSynchronize());
        t_kernel.stop();
        t_down.start();                               // 力和势能拷回来
        CHECK(cudaMemcpy(s.fx.data(), d_fx, n * sizeof(double), cudaMemcpyDeviceToHost));
        CHECK(cudaMemcpy(s.fy.data(), d_fy, n * sizeof(double), cudaMemcpyDeviceToHost));
        CHECK(cudaMemcpy(s.fz.data(), d_fz, n * sizeof(double), cudaMemcpyDeviceToHost));
        CHECK(cudaMemcpy(s.epot.data(), d_e, n * sizeof(double), cudaMemcpyDeviceToHost));
        t_down.stop();
    };
    // ANCHOR_END: step

    rebuild();
    compute_forces();
    double e0 = kinetic(s) + potential(s);
    printf("  步数     动能/N     势能/N     总能量/N\n");
    printf("%6d  %9.5f  %9.5f  %10.6f\n", 0, kinetic(s) / n, potential(s) / n, e0 / n);
    for (int step = 1; step <= p.steps; ++step) {
        t_int.start();
        kick(s, 0.5 * p.dt);
        drift(s, p.dt);
        t_int.stop();
        if (step % p.rebuild_every == 0) rebuild();
        compute_forces();
        t_int.start();
        kick(s, 0.5 * p.dt);
        t_int.stop();
        if (step % p.report_every == 0) {
            double ke = kinetic(s), pe = potential(s);
            printf("%6d  %9.5f  %9.5f  %10.6f\n", step, ke / n, pe / n, (ke + pe) / n);
        }
    }
    double e1 = kinetic(s) + potential(s);
    printf("总能量相对漂移 %.1e\n\n", std::fabs(e1 - e0) / std::fabs(e0));
    double total = t_nbr.total + t_up.total + t_kernel.total + t_down.total + t_int.total;
    auto row = [&](const char* name, double ms) {
        printf("%-22s %9.1f ms  %5.1f%%  %6.2f ms\n", name, ms, 100 * ms / total, ms / p.steps);
    };
    printf("阶段                       时间        占比    每步\n");
    row("求力 kernel", t_kernel.total);
    row("位置拷上去", t_up.total);
    row("力拷回来", t_down.total);
    row("邻居表（CPU，含拷贝）", t_nbr.total);
    row("积分（CPU）", t_int.total);
    printf("%-22s %9.1f ms          %6.2f ms\n", "合计", total, total / p.steps);
}
