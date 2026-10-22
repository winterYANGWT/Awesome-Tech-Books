// md_gpu.cu: 分子动力学的全部计算都在 GPU 上：建邻居表、求力、积分，数据一直留在显存里，
// 只在报告能量时拷回两个数。
//   nvcc -O2 -arch=native md_gpu.cu -o md_gpu && ./md_gpu [lattice|shuffled|reorder] [晶胞数] [步数]
//   lattice：粒子按初始晶格的顺序编号；shuffled：初始时把粒子编号随机打乱；
//   reorder：同样随机打乱，但每次重建邻居表时把粒子按格子顺序重新排列。
#include "../ch27/md_common.h"
#include <cub/cub.cuh>
#include <algorithm>
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

constexpr int MAX_NBR = 128;

// 显存里的全部状态。
struct Dev {
    int n, nc;                     // 粒子数，每个方向的格子数
    double box, cell;              // 盒长，格子边长
    double *x, *y, *z, *vx, *vy, *vz, *fx, *fy, *fz, *epot;
    double *tmp;                   // 重新排列时用的临时数组
    int *cell_of, *ids, *cell_sorted, *ids_sorted;   // 每个粒子的格子号、按格子排序后的结果
    int *cell_start, *cell_end;    // 每个格子在排序结果里的起止位置
    int *nbr_count, *nbr;          // 邻居表，转置存放：nbr[k * n + i]
    double* energy;                // [0] 动能，[1] 势能
};

__global__ void kick_kernel(Dev d, double half_dt) {
    int i = blockIdx.x * blockDim.x + threadIdx.x;
    if (i >= d.n) return;
    d.vx[i] += half_dt * d.fx[i], d.vy[i] += half_dt * d.fy[i], d.vz[i] += half_dt * d.fz[i];
}

__global__ void drift_kernel(Dev d, double dt) {
    int i = blockIdx.x * blockDim.x + threadIdx.x;
    if (i >= d.n) return;
    double x = d.x[i] + dt * d.vx[i], y = d.y[i] + dt * d.vy[i], z = d.z[i] + dt * d.vz[i];
    d.x[i] = x - d.box * floor(x / d.box), d.y[i] = y - d.box * floor(y / d.box), d.z[i] = z - d.box * floor(z / d.box);
}

__device__ int cell_coord(double v, double cell, int nc) {
    int c = int(v / cell);
    return c < 0 ? 0 : c >= nc ? nc - 1 : c;
}

// ANCHOR: cells
// 第一步：算出每个粒子所在的格子。
__global__ void cell_index_kernel(Dev d) {
    int i = blockIdx.x * blockDim.x + threadIdx.x;
    if (i >= d.n) return;
    int cx = cell_coord(d.x[i], d.cell, d.nc), cy = cell_coord(d.y[i], d.cell, d.nc), cz = cell_coord(d.z[i], d.cell, d.nc);
    d.cell_of[i] = (cx * d.nc + cy) * d.nc + cz;
    d.ids[i] = i;
}

// 第三步：按格子号排好序以后，找出每个格子在排序结果里从哪里开始、到哪里结束。
__global__ void cell_bounds_kernel(Dev d) {
    int s = blockIdx.x * blockDim.x + threadIdx.x;
    if (s >= d.n) return;
    int c = d.cell_sorted[s];
    if (s == 0 || d.cell_sorted[s - 1] != c) d.cell_start[c] = s;
    if (s == d.n - 1 || d.cell_sorted[s + 1] != c) d.cell_end[c] = s + 1;
}

// 第四步：一个线程负责一个粒子，查自己和周围 26 个格子里的粒子。
__global__ void neighbor_kernel(Dev d, float rlist2) {
    int i = blockIdx.x * blockDim.x + threadIdx.x;
    if (i >= d.n) return;
    float box = float(d.box), inv_box = float(1.0 / d.box);
    double xi = d.x[i], yi = d.y[i], zi = d.z[i];
    int cx = cell_coord(xi, d.cell, d.nc), cy = cell_coord(yi, d.cell, d.nc), cz = cell_coord(zi, d.cell, d.nc);
    int count = 0;
    for (int dx = -1; dx <= 1; ++dx)
        for (int dy = -1; dy <= 1; ++dy)
            for (int dz = -1; dz <= 1; ++dz) {
                int c = (((cx + dx + d.nc) % d.nc) * d.nc + (cy + dy + d.nc) % d.nc) * d.nc + (cz + dz + d.nc) % d.nc;
                for (int s = d.cell_start[c]; s < d.cell_end[c]; ++s) {
                    int j = d.ids_sorted[s];
                    if (j == i) continue;
                    float rx = float(xi - d.x[j]), ry = float(yi - d.y[j]), rz = float(zi - d.z[j]);
                    rx -= box * rintf(rx * inv_box), ry -= box * rintf(ry * inv_box), rz -= box * rintf(rz * inv_box);
                    if (rx * rx + ry * ry + rz * rz < rlist2 && count < MAX_NBR) d.nbr[count++ * d.n + i] = j;
                }
            }
    d.nbr_count[i] = count;
}
// ANCHOR_END: cells

// 把一个数组按排序结果重新排列：新的第 s 个元素是原来的第 ids_sorted[s] 个。
__global__ void gather_kernel(const double* src, double* dst, const int* order, int n) {
    int s = blockIdx.x * blockDim.x + threadIdx.x;
    if (s < n) dst[s] = src[order[s]];
}

__global__ void iota_kernel(int* a, int n) {
    int i = blockIdx.x * blockDim.x + threadIdx.x;
    if (i < n) a[i] = i;
}

// 第 28 章的混合精度求力 kernel。
__global__ void forces_kernel(Dev d, float rc2, float e_shift) {
    int i = blockIdx.x * blockDim.x + threadIdx.x;
    if (i >= d.n) return;
    double xi = d.x[i], yi = d.y[i], zi = d.z[i];
    float box = float(d.box), inv_box = float(1.0 / d.box);
    float ax = 0, ay = 0, az = 0, e = 0;
    for (int k = 0; k < d.nbr_count[i]; ++k) {
        int j = d.nbr[k * d.n + i];
        float dx = float(xi - d.x[j]), dy = float(yi - d.y[j]), dz = float(zi - d.z[j]);
        dx -= box * rintf(dx * inv_box), dy -= box * rintf(dy * inv_box), dz -= box * rintf(dz * inv_box);
        float r2 = dx * dx + dy * dy + dz * dz;
        if (r2 >= rc2) continue;
        float inv_r2 = 1.0f / r2, inv_r6 = inv_r2 * inv_r2 * inv_r2;
        float f = 24.0f * inv_r2 * inv_r6 * (2.0f * inv_r6 - 1.0f);
        ax += f * dx, ay += f * dy, az += f * dz;
        e += 0.5f * (4.0f * inv_r6 * (inv_r6 - 1.0f) - e_shift);
    }
    d.fx[i] = ax, d.fy[i] = ay, d.fz[i] = az, d.epot[i] = e;
}

__global__ void energy_kernel(Dev d) {
    double ke = 0, pe = 0;
    for (int i = blockIdx.x * blockDim.x + threadIdx.x; i < d.n; i += gridDim.x * blockDim.x) {
        ke += 0.5 * (d.vx[i] * d.vx[i] + d.vy[i] * d.vy[i] + d.vz[i] * d.vz[i]);
        pe += d.epot[i];
    }
    atomicAdd(&d.energy[0], ke);
    atomicAdd(&d.energy[1], pe);
}

int main(int argc, char** argv) {
    const char* mode = argc > 1 ? argv[1] : "lattice";
    bool reorder = !strcmp(mode, "reorder"), shuffle = reorder || !strcmp(mode, "shuffled");
    Params p;
    if (argc > 2) p.nc = atoi(argv[2]);
    if (argc > 3) p.steps = atoi(argv[3]);
    System s;
    init_system(s, p);
    const int n = s.n, threads = 128, blocks = (n + threads - 1) / threads;
    // ANCHOR: shuffle
    if (shuffle) {                     // 把粒子编号随机打乱：空间上相邻的粒子，在数组里不再相邻
        std::vector<int> perm(n);
        for (int i = 0; i < n; ++i) perm[i] = i;
        std::shuffle(perm.begin(), perm.end(), std::mt19937(99));
        for (auto* v : {&s.x, &s.y, &s.z, &s.vx, &s.vy, &s.vz}) {
            std::vector<double> t(n);
            for (int i = 0; i < n; ++i) t[i] = (*v)[perm[i]];
            *v = t;
        }
    }
    // ANCHOR_END: shuffle
    const double rlist = p.rc + p.skin, rc2 = p.rc * p.rc;
    const double inv_rc6 = 1.0 / (rc2 * rc2 * rc2), e_shift = 4.0 * inv_rc6 * (inv_rc6 - 1.0);

    Dev d;
    d.n = n, d.box = s.box, d.nc = int(s.box / rlist), d.cell = s.box / d.nc;
    const int ncells = d.nc * d.nc * d.nc;
    for (double** a : {&d.x, &d.y, &d.z, &d.vx, &d.vy, &d.vz, &d.fx, &d.fy, &d.fz, &d.epot, &d.tmp})
        CHECK(cudaMalloc(a, n * sizeof(double)));
    for (int** a : {&d.cell_of, &d.ids, &d.cell_sorted, &d.ids_sorted, &d.nbr_count})
        CHECK(cudaMalloc(a, n * sizeof(int)));
    CHECK(cudaMalloc(&d.cell_start, ncells * sizeof(int)));
    CHECK(cudaMalloc(&d.cell_end, ncells * sizeof(int)));
    CHECK(cudaMalloc(&d.nbr, size_t(n) * MAX_NBR * sizeof(int)));
    CHECK(cudaMalloc(&d.energy, 2 * sizeof(double)));
    // 初始状态只拷一次。
    double* dst[] = {d.x, d.y, d.z, d.vx, d.vy, d.vz};
    std::vector<double>* src[] = {&s.x, &s.y, &s.z, &s.vx, &s.vy, &s.vz};
    for (int k = 0; k < 6; ++k) CHECK(cudaMemcpy(dst[k], src[k]->data(), n * sizeof(double), cudaMemcpyHostToDevice));
    void* sort_temp = nullptr;
    size_t sort_bytes = 0;
    CHECK(cub::DeviceRadixSort::SortPairs(sort_temp, sort_bytes, d.cell_of, d.cell_sorted, d.ids, d.ids_sorted, n));
    CHECK(cudaMalloc(&sort_temp, sort_bytes));
    const char* desc = reorder ? "初始随机打乱，重建邻居表时按格子重新排列"
                     : shuffle ? "初始随机打乱，之后顺序不变" : "按初始晶格顺序编号";
    printf("%d 个粒子，%d 步，%d×%d×%d 个格子，%s\n", n, p.steps, d.nc, d.nc, d.nc, desc);

    // ANCHOR: rebuild
    auto rebuild = [&] {
        cell_index_kernel<<<blocks, threads>>>(d);
        CHECK(cub::DeviceRadixSort::SortPairs(sort_temp, sort_bytes, d.cell_of, d.cell_sorted, d.ids,
                                              d.ids_sorted, n));        // 第二步：按格子号排序
        CHECK(cudaMemsetAsync(d.cell_start, 0, ncells * sizeof(int)));
        CHECK(cudaMemsetAsync(d.cell_end, 0, ncells * sizeof(int)));
        cell_bounds_kernel<<<blocks, threads>>>(d);
        if (reorder) {
            // 按格子顺序重新排列所有粒子数组：空间上相邻的粒子，在内存里也相邻。
            for (double** a : {&d.x, &d.y, &d.z, &d.vx, &d.vy, &d.vz, &d.fx, &d.fy, &d.fz}) {
                gather_kernel<<<blocks, threads>>>(*a, d.tmp, d.ids_sorted, n);
                std::swap(*a, d.tmp);
            }
            iota_kernel<<<blocks, threads>>>(d.ids_sorted, n);   // 排列之后，第 s 个就是粒子 s
        }
        neighbor_kernel<<<blocks, threads>>>(d, float(rlist * rlist));
    };
    // ANCHOR_END: rebuild
    auto forces = [&] { forces_kernel<<<blocks, threads>>>(d, float(rc2), float(e_shift)); };
    auto report = [&](int step, double& e) {
        CHECK(cudaMemsetAsync(d.energy, 0, 2 * sizeof(double)));
        energy_kernel<<<26 * 6, 256>>>(d);
        double h[2];
        CHECK(cudaMemcpy(h, d.energy, sizeof(h), cudaMemcpyDeviceToHost));   // 每次报告只拷回两个数
        e = h[0] + h[1];
        printf("%6d  %9.5f  %9.5f  %10.6f\n", step, h[0] / n, h[1] / n, e / n);
    };

    rebuild();
    forces();
    double e0, e1;
    printf("  步数     动能/N     势能/N     总能量/N\n");
    report(0, e0);
    // 计时用的 event：每个阶段前后各记一个，最后统一读出。
    cudaEvent_t ev[4];
    for (auto& e : ev) CHECK(cudaEventCreate(&e));
    float t_nbr = 0, t_force = 0, t_int = 0;
    CHECK(cudaDeviceSynchronize());
    auto wall0 = std::chrono::steady_clock::now();
    // ANCHOR: loop
    for (int step = 1; step <= p.steps; ++step) {
        CHECK(cudaEventRecord(ev[0]));
        kick_kernel<<<blocks, threads>>>(d, 0.5 * p.dt);
        drift_kernel<<<blocks, threads>>>(d, p.dt);
        CHECK(cudaEventRecord(ev[1]));
        if (step % p.rebuild_every == 0) rebuild();
        CHECK(cudaEventRecord(ev[2]));
        forces();
        kick_kernel<<<blocks, threads>>>(d, 0.5 * p.dt);
        CHECK(cudaEventRecord(ev[3]));
        CHECK(cudaEventSynchronize(ev[3]));
        float a, b, c;
        CHECK(cudaEventElapsedTime(&a, ev[0], ev[1]));
        CHECK(cudaEventElapsedTime(&b, ev[1], ev[2]));
        CHECK(cudaEventElapsedTime(&c, ev[2], ev[3]));
        t_int += a, t_nbr += b, t_force += c;
        if (step % p.report_every == 0) report(step, e1);
    }
    // ANCHOR_END: loop
    CHECK(cudaGetLastError());
    double wall = std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - wall0).count();
    printf("总能量相对漂移 %.1e\n\n", std::fabs(e1 - e0) / std::fabs(e0));
    printf("阶段                      GPU 时间    每步\n");
    printf("求力（含后半步 kick）  %9.1f ms  %6.3f ms\n", t_force, t_force / p.steps);
    printf("邻居表               %9.1f ms  %6.3f ms\n", t_nbr, t_nbr / p.steps);
    printf("积分（前半步）        %9.1f ms  %6.3f ms\n", t_int, t_int / p.steps);
    printf("整个循环（墙钟时间）  %9.1f ms  %6.3f ms\n", wall, wall / p.steps);
}
