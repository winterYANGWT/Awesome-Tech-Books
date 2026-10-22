// layout.cu: 同样的粒子数据，按结构体数组（AoS）和数组结构体（SoA）两种方式存放，
// 跑两个 kernel：一个用到 8 个字段里的 6 个，一个只用 1 个。
//   nvcc -O2 -arch=native layout.cu -o layout && ./layout
#include <cstdio>
#include <cstdlib>

#define CHECK(call)                                                        \
    do {                                                                   \
        cudaError_t err_ = (call);                                         \
        if (err_ != cudaSuccess) {                                         \
            fprintf(stderr, "%s:%d: %s (%s)\n", __FILE__, __LINE__,        \
                    cudaGetErrorName(err_), cudaGetErrorString(err_));     \
            exit(1);                                                       \
        }                                                                  \
    } while (0)

// ANCHOR: layouts
// AoS：一个粒子的所有字段挨在一起，32 字节一个粒子。
struct Particle {
    float x, y, z;
    float vx, vy, vz;
    float mass, charge;
};

// SoA：每个字段一个数组。
struct Particles {
    float *x, *y, *z;
    float *vx, *vy, *vz;
    float *mass, *charge;
};
// ANCHOR_END: layouts

// ANCHOR: kernels
// 按速度移动粒子：读 6 个字段，写 3 个。
__global__ void drift_aos(Particle* p, int n, float dt) {
    int i = blockIdx.x * blockDim.x + threadIdx.x;
    if (i >= n) return;
    p[i].x += p[i].vx * dt;
    p[i].y += p[i].vy * dt;
    p[i].z += p[i].vz * dt;
}

__global__ void drift_soa(Particles p, int n, float dt) {
    int i = blockIdx.x * blockDim.x + threadIdx.x;
    if (i >= n) return;
    p.x[i] += p.vx[i] * dt;
    p.y[i] += p.vy[i] * dt;
    p.z[i] += p.vz[i] * dt;
}

// 统计质量超过阈值的粒子：只读 1 个字段。
__global__ void heavy_aos(const Particle* p, int n, float limit, int* count) {
    int i = blockIdx.x * blockDim.x + threadIdx.x;
    if (i < n && p[i].mass > limit) atomicAdd(count, 1);
}

__global__ void heavy_soa(const float* mass, int n, float limit, int* count) {
    int i = blockIdx.x * blockDim.x + threadIdx.x;
    if (i < n && mass[i] > limit) atomicAdd(count, 1);
}
// ANCHOR_END: kernels

int main() {
    const int n = 1 << 24;                    // 1600 多万个粒子，每个字段数组 64 MB，超过 L2
    Particle* aos;
    Particles soa;
    int* count;
    CHECK(cudaMalloc(&aos, n * sizeof(Particle)));
    CHECK(cudaMemset(aos, 0, n * sizeof(Particle)));
    float** fields[] = {&soa.x, &soa.y, &soa.z, &soa.vx, &soa.vy, &soa.vz, &soa.mass, &soa.charge};
    for (float** f : fields) {
        CHECK(cudaMalloc(f, n * sizeof(float)));
        CHECK(cudaMemset(*f, 0, n * sizeof(float)));
    }
    CHECK(cudaMalloc(&count, sizeof(int)));
    cudaEvent_t start, stop;
    CHECK(cudaEventCreate(&start));
    CHECK(cudaEventCreate(&stop));
    auto time = [&](auto launch) {
        launch();
        CHECK(cudaGetLastError());
        float best = 1e30f;
        for (int r = 0; r < 5; ++r) {
            CHECK(cudaEventRecord(start));
            launch();
            CHECK(cudaEventRecord(stop));
            CHECK(cudaEventSynchronize(stop));
            float ms;
            CHECK(cudaEventElapsedTime(&ms, start, stop));
            if (ms < best) best = ms;
        }
        return best;
    };
    int blocks = (n + 255) / 256;
    float da = time([&] { drift_aos<<<blocks, 256>>>(aos, n, 0.01f); });
    float ds = time([&] { drift_soa<<<blocks, 256>>>(soa, n, 0.01f); });
    float ha = time([&] { heavy_aos<<<blocks, 256>>>(aos, n, 1.0f, count); });
    float hs = time([&] { heavy_soa<<<blocks, 256>>>(soa.mass, n, 1.0f, count); });
    printf("            AoS        SoA\n");
    printf("drift   %6.2f ms  %6.2f ms\n", da, ds);
    printf("heavy   %6.2f ms  %6.2f ms\n", ha, hs);
    CHECK(cudaFree(aos));
    for (float** f : fields) CHECK(cudaFree(*f));
    CHECK(cudaFree(count));
}
