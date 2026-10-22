// spill.cu: 寄存器不够用时，变量会被挤到 local memory。
// 同一段计算编成两个 kernel：一个寄存器随便用，一个被 __launch_bounds__ 限制得很紧。
// 再加一个用变量下标访问局部数组的 kernel。
//   nvcc -O2 -arch=native -Xptxas -v spill.cu -o spill && ./spill
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

constexpr int R = 48;          // 同时活着的累加器个数

// ANCHOR: body
// 48 个累加器互相依赖，整个循环里都要保持活着。
__device__ float mix(float x) {
    float acc[R];
#pragma unroll
    for (int j = 0; j < R; ++j) acc[j] = x + j;
    for (int k = 0; k < 200; ++k) {
#pragma unroll
        for (int j = 0; j < R; ++j) acc[j] = acc[j] * 0.999f + acc[(j + 1) % R] * 0.001f;
    }
    float s = 0;
#pragma unroll
    for (int j = 0; j < R; ++j) s += acc[j];
    return s;
}

// 不限制：编译器想用多少寄存器就用多少。
__global__ void __launch_bounds__(256, 1) mix_free(const float* in, float* out) {
    int t = blockIdx.x * blockDim.x + threadIdx.x;
    out[t] = mix(in[t]);
}

// 要求每个 SM 能同时放下 6 个 256 线程的 block：65536 个寄存器分给 1536 个线程，
// 每个线程最多 40 个出头，放不下 48 个累加器。
__global__ void __launch_bounds__(256, 6) mix_tight(const float* in, float* out) {
    int t = blockIdx.x * blockDim.x + threadIdx.x;
    out[t] = mix(in[t]);
}
// ANCHOR_END: body

// ANCHOR: dynamic
// 每个线程在局部数组里统计一个小直方图。写入的下标运行时才知道，
// 寄存器又没有“第 k 个寄存器”这种寻址方式，数组只能放进 local memory。
__global__ void histogram(const int* keys, int* out, int n) {
    int t = blockIdx.x * blockDim.x + threadIdx.x;
    int bins[32] = {};
    for (int j = 0; j < 64; ++j) bins[keys[(t + j * 4099) % n] & 31]++;
    int best = 0;
    for (int b = 1; b < 32; ++b) best = bins[b] > bins[best] ? b : best;
    out[t] = best;
}
// ANCHOR_END: dynamic

int main() {
    const int blocks = 26 * 6 * 8, threads = 256, n = blocks * threads;
    float *in, *out;
    int* idx;
    CHECK(cudaMalloc(&in, n * sizeof(float)));
    CHECK(cudaMalloc(&out, n * sizeof(float)));
    CHECK(cudaMalloc(&idx, n * sizeof(int)));
    CHECK(cudaMemset(in, 0, n * sizeof(float)));
    CHECK(cudaMemset(idx, 0, n * sizeof(int)));
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
    float f = time([&] { mix_free<<<blocks, threads>>>(in, out); });
    float t = time([&] { mix_tight<<<blocks, threads>>>(in, out); });
    printf("mix_free   %6.2f ms\nmix_tight  %6.2f ms\n", f, t);
    CHECK(cudaFree(in));
    CHECK(cudaFree(out));
    CHECK(cudaFree(idx));
}
