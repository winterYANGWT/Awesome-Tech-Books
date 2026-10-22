// fp_rate.cu: 测 FP32、FP64、FP16、BF16 乘加（不用张量核）的峰值吞吐。
//   nvcc -O2 -arch=native fp_rate.cu -o fp_rate && ./fp_rate
#include <cuda_bf16.h>
#include <cuda_fp16.h>
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

// ANCHOR: kernel
// 每个线程 8 条互不依赖的乘加链，每条做 iters 次。T 是 half2 或 bfloat162 时，一条指令做两个乘加。
template <class T>
__global__ void fma_chain(T* out, T b, T c, int iters) {
    T a[8];
    a[0] = b;
    for (int j = 1; j < 8; ++j) a[j] = a[j - 1] * b + c;   // 8 个不同的初值
    for (int i = 0; i < iters; ++i)
#pragma unroll
        for (int j = 0; j < 8; ++j) a[j] = a[j] * b + c;
    T s = a[0];
    for (int j = 1; j < 8; ++j) s = s + a[j];
    out[blockIdx.x * blockDim.x + threadIdx.x] = s;   // 写出结果，免得整个循环被优化掉
}
// ANCHOR_END: kernel

template <class T>
double tflops(const char* name, T b, T c, int iters, int lanes) {
    const int threads = 26 * 1536 * 4;
    T* out;
    CHECK(cudaMalloc(&out, threads * sizeof(T)));
    cudaEvent_t e0, e1;
    CHECK(cudaEventCreate(&e0));
    CHECK(cudaEventCreate(&e1));
    fma_chain<<<threads / 256, 256>>>(out, b, c, iters);
    CHECK(cudaEventRecord(e0));
    fma_chain<<<threads / 256, 256>>>(out, b, c, iters);
    CHECK(cudaEventRecord(e1));
    CHECK(cudaEventSynchronize(e1));
    float ms;
    CHECK(cudaEventElapsedTime(&ms, e0, e1));
    double t = 2.0 * 8 * lanes * iters * threads / ms / 1e9;
    printf("%-8s %8.2f TFLOPS\n", name, t);
    CHECK(cudaFree(out));
    return t;
}

int main() {
    double f32 = tflops<float>("FP32", 0.999f, 0.001f, 4096, 1);
    double f64 = tflops<double>("FP64", 0.999, 0.001, 256, 1);
    tflops<__half2>("FP16x2", __float2half2_rn(0.999f), __float2half2_rn(0.001f), 4096, 2);
    tflops<__nv_bfloat162>("BF16x2", __float2bfloat162_rn(0.999f), __float2bfloat162_rn(0.001f), 4096, 2);
    printf("FP64 / FP32 = 1 / %.0f\n", f32 / f64);
}
