// fma.cu: a * b + c 在 GPU 上默认编译成一条 FMA（中间不舍入），在 CPU 上默认是先乘后加（舍入两次）。
// 用 __fmul_rn / __fadd_rn 可以在 GPU 上逐处禁止合并；CPU 上用 fmaf 可以得到和 GPU 一样的结果。
//   nvcc -O2 -arch=native fma.cu -o fma && ./fma
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <random>

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
__global__ void muladd(const float* a, const float* b, const float* c, float* fused, float* separate, int n) {
    int i = blockIdx.x * blockDim.x + threadIdx.x;
    if (i >= n) return;
    fused[i] = a[i] * b[i] + c[i];                    // 默认会被合并成 FMA
    separate[i] = __fadd_rn(__fmul_rn(a[i], b[i]), c[i]);   // 这两个函数保证不参与合并
}
// ANCHOR_END: kernel

int main() {
    const int n = 1 << 22;
    float *a, *b, *c, *fused, *separate;
    for (float** p : {&a, &b, &c, &fused, &separate}) CHECK(cudaMallocManaged(p, n * sizeof(float)));
    std::mt19937 rng(3);
    std::uniform_real_distribution<float> u(-1, 1);
    for (int i = 0; i < n; ++i) a[i] = u(rng), b[i] = u(rng), c[i] = u(rng);
    muladd<<<n / 256, 256>>>(a, b, c, fused, separate, n);
    CHECK(cudaDeviceSynchronize());
    // ANCHOR: compare
    int d_plain_fused = 0, d_plain_sep = 0, d_fmaf_fused = 0;
    for (int i = 0; i < n; ++i) {
        volatile float prod = a[i] * b[i];            // volatile：确保主机编译器先舍入乘积再加
        float plain = prod + c[i];
        float f = fmaf(a[i], b[i], c[i]);             // C 标准库的 FMA，只舍入一次
        d_plain_fused += plain != fused[i];
        d_plain_sep += plain != separate[i];
        d_fmaf_fused += f != fused[i];
    }
    // ANCHOR_END: compare
    printf("%d 组 [-1,1) 之间的 a、b、c：\n", n);
    printf("  CPU 先乘后加 vs GPU 默认（FMA）       不同 %6.2f%%\n", 100.0 * d_plain_fused / n);
    printf("  CPU 先乘后加 vs GPU __fmul_rn/__fadd_rn 不同 %6.2f%%\n", 100.0 * d_plain_sep / n);
    printf("  CPU fmaf     vs GPU 默认（FMA）       不同 %6.2f%%\n", 100.0 * d_fmaf_fused / n);
}
