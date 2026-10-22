// constant.cu: 同一张系数表放在 constant memory 和 global memory 里，
// 一个 warp 的线程读同一个系数和各读各的系数，速度各是多少。
//   nvcc -O2 -arch=native constant.cu -o constant && ./constant
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

constexpr int TERMS = 256;
// ANCHOR: decl
__constant__ float c_coef[TERMS];      // 放在 constant memory 里，kernel 只能读
// ANCHOR_END: decl

// ANCHOR: kernels
// 用 TERMS 个系数算一个多项式。spread 为假时，同一时刻所有线程读同一个系数；
// spread 为真时，每个线程从不同的位置开始读，同一时刻 32 个线程读 32 个不同的系数。
__global__ void poly_constant(float* out, bool spread) {
    int t = blockIdx.x * blockDim.x + threadIdx.x;
    float x = 0.001f * (t % 1000), v = 0.0f;
    for (int k = 0; k < TERMS; ++k) {
        int j = spread ? (k + threadIdx.x) % TERMS : k;
        v = v * x + c_coef[j];
    }
    out[t] = v;
}

__global__ void poly_global(const float* __restrict__ coef, float* out, bool spread) {
    int t = blockIdx.x * blockDim.x + threadIdx.x;
    float x = 0.001f * (t % 1000), v = 0.0f;
    for (int k = 0; k < TERMS; ++k) {
        int j = spread ? (k + threadIdx.x) % TERMS : k;
        v = v * x + coef[j];
    }
    out[t] = v;
}
// ANCHOR_END: kernels

int main() {
    float h[TERMS];
    for (int k = 0; k < TERMS; ++k) h[k] = 1.0f / (k + 1);
    // ANCHOR: copy
    CHECK(cudaMemcpyToSymbol(c_coef, h, sizeof(h)));   // 主机端用 cudaMemcpyToSymbol 填充
    // ANCHOR_END: copy
    float *g_coef, *out;
    const int blocks = 26 * 6 * 16, threads = 256;
    CHECK(cudaMalloc(&g_coef, sizeof(h)));
    CHECK(cudaMemcpy(g_coef, h, sizeof(h), cudaMemcpyHostToDevice));
    CHECK(cudaMalloc(&out, blocks * threads * sizeof(float)));
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
    float cu = time([&] { poly_constant<<<blocks, threads>>>(out, false); });
    float cs = time([&] { poly_constant<<<blocks, threads>>>(out, true); });
    float gu = time([&] { poly_global<<<blocks, threads>>>(g_coef, out, false); });
    float gs = time([&] { poly_global<<<blocks, threads>>>(g_coef, out, true); });
    printf("                 同一个系数   各读各的\n");
    printf("constant memory  %7.2f ms  %7.2f ms\n", cu, cs);
    printf("global memory    %7.2f ms  %7.2f ms\n", gu, gs);
    CHECK(cudaFree(g_coef));
    CHECK(cudaFree(out));
}
