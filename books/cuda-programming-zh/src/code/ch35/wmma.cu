// wmma.cu: 用 WMMA API 调用张量核做半精度矩阵乘（float 累加），和 cuBLAS 比较。
//   nvcc -O2 -arch=native wmma.cu -lcublas -o wmma && ./wmma
// NVCCFLAGS: -lcublas
#include <cublas_v2.h>
#include <cuda_fp16.h>
#include <mma.h>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <vector>
using namespace nvcuda;

#define CHECK(call)                                                        \
    do {                                                                   \
        cudaError_t err_ = (call);                                         \
        if (err_ != cudaSuccess) {                                         \
            fprintf(stderr, "%s:%d: %s (%s)\n", __FILE__, __LINE__,        \
                    cudaGetErrorName(err_), cudaGetErrorString(err_));     \
            exit(1);                                                       \
        }                                                                  \
    } while (0)

const int M = 4096, N = 4096, K = 4096;              // A 是 M×K，B 是 K×N，都按行存放

// ANCHOR: simple
// 每个 warp 算 C 的一个 16×16 块：沿 K 方向每次从显存装入 A、B 各一个 16×16 的片段，在张量核上乘加。
__global__ void wmma_simple(const half* A, const half* B, float* C) {
    int warp = (blockIdx.x * blockDim.x + threadIdx.x) / 32;
    int tile_m = warp / (N / 16), tile_n = warp % (N / 16);
    wmma::fragment<wmma::matrix_a, 16, 16, 16, half, wmma::row_major> a;
    wmma::fragment<wmma::matrix_b, 16, 16, 16, half, wmma::row_major> b;
    wmma::fragment<wmma::accumulator, 16, 16, 16, float> c;
    wmma::fill_fragment(c, 0.0f);
    for (int k = 0; k < K; k += 16) {
        wmma::load_matrix_sync(a, A + size_t(tile_m) * 16 * K + k, K);   // 最后一个参数是行跨度
        wmma::load_matrix_sync(b, B + size_t(k) * N + tile_n * 16, N);
        wmma::mma_sync(c, a, b, c);                   // c += a * b，整个 warp 一起执行
    }
    wmma::store_matrix_sync(C + size_t(tile_m) * 16 * N + tile_n * 16, c, N, wmma::mem_row_major);
}
// ANCHOR_END: simple

// ANCHOR: tiled
// 每个 block 4 个 warp，算 C 的 64×64 块；每个 warp 算其中 32×32（2×2 个片段）。
// 沿 K 方向每次把 A 的 64×32 和 B 的 32×64 搬进 shared memory，片段从 shared memory 装入。
const int BT = 64, KT = 32, PAD = 8;                  // 每行多留 8 个 half，减少 bank conflict
__global__ void wmma_tiled(const half* A, const half* B, float* C) {
    __shared__ __align__(16) half As[BT][KT + PAD];
    __shared__ __align__(16) half Bs[KT][BT + PAD];
    int warp = threadIdx.x / 32, wm = warp / 2, wn = warp % 2;   // warp 在 block 里的位置（2×2）
    int row0 = blockIdx.y * BT, col0 = blockIdx.x * BT;
    wmma::fragment<wmma::matrix_a, 16, 16, 16, half, wmma::row_major> a[2];
    wmma::fragment<wmma::matrix_b, 16, 16, 16, half, wmma::row_major> b[2];
    wmma::fragment<wmma::accumulator, 16, 16, 16, float> c[2][2];
    for (auto& r : c)
        for (auto& f : r) wmma::fill_fragment(f, 0.0f);
    for (int k0 = 0; k0 < K; k0 += KT) {
        // 128 个线程每次搬 8 个 half（16 字节）：A 块 64×32 和 B 块 32×64 各要搬两轮
        for (int i = threadIdx.x; i < BT * KT / 8; i += blockDim.x) {
            int r = i / (KT / 8), cc = i % (KT / 8) * 8;
            *(int4*)&As[r][cc] = *(const int4*)&A[size_t(row0 + r) * K + k0 + cc];
            int r2 = i / (BT / 8), c2 = i % (BT / 8) * 8;
            *(int4*)&Bs[r2][c2] = *(const int4*)&B[size_t(k0 + r2) * N + col0 + c2];
        }
        __syncthreads();
        for (int kk = 0; kk < KT; kk += 16) {
            for (int i = 0; i < 2; ++i) wmma::load_matrix_sync(a[i], &As[wm * 32 + i * 16][kk], KT + PAD);
            for (int j = 0; j < 2; ++j) wmma::load_matrix_sync(b[j], &Bs[kk][wn * 32 + j * 16], BT + PAD);
            for (int i = 0; i < 2; ++i)
                for (int j = 0; j < 2; ++j) wmma::mma_sync(c[i][j], a[i], b[j], c[i][j]);
        }
        __syncthreads();
    }
    for (int i = 0; i < 2; ++i)
        for (int j = 0; j < 2; ++j)
            wmma::store_matrix_sync(C + size_t(row0 + wm * 32 + i * 16) * N + col0 + wn * 32 + j * 16, c[i][j], N,
                                    wmma::mem_row_major);
}
// ANCHOR_END: tiled

__global__ void init(half* p, size_t n, unsigned seed) {
    for (size_t i = blockIdx.x * size_t(blockDim.x) + threadIdx.x; i < n; i += size_t(gridDim.x) * blockDim.x) {
        unsigned x = unsigned(i) * 2654435761u + seed;
        x = (x ^ (x >> 15)) * 0x2c1b3c6du;
        p[i] = __float2half(float(x >> 8) / 16777216.0f - 0.5f);   // [-0.5, 0.5)
    }
}

template <class F>
float time_ms(F f) {
    cudaEvent_t e0, e1;
    CHECK(cudaEventCreate(&e0));
    CHECK(cudaEventCreate(&e1));
    f();                                              // 预热
    CHECK(cudaEventRecord(e0));
    for (int r = 0; r < 5; ++r) f();
    CHECK(cudaEventRecord(e1));
    CHECK(cudaEventSynchronize(e1));
    float ms;
    CHECK(cudaEventElapsedTime(&ms, e0, e1));
    return ms / 5;
}

int main() {
    half *A, *B;
    float *C1, *C2, *Cref;
    CHECK(cudaMalloc(&A, size_t(M) * K * sizeof(half)));
    CHECK(cudaMalloc(&B, size_t(K) * N * sizeof(half)));
    for (float** p : {&C1, &C2, &Cref}) CHECK(cudaMalloc(p, size_t(M) * N * sizeof(float)));
    init<<<1024, 256>>>(A, size_t(M) * K, 1);
    init<<<1024, 256>>>(B, size_t(K) * N, 2);
    double flop = 2.0 * M * N * K;

    float t1 = time_ms([&] { wmma_simple<<<M / 16 * N / 16 / 4, 128>>>(A, B, C1); });
    float t2 = time_ms([&] { wmma_tiled<<<dim3(N / BT, M / BT), 128>>>(A, B, C2); });
    // ANCHOR: cublas
    cublasHandle_t h;
    cublasCreate(&h);
    float one = 1, zero = 0;
    // cuBLAS 按列存放：行存放的 C = A·B 就是列存放的 Cᵀ = Bᵀ·Aᵀ，所以把 B 放在前面
    float t3 = time_ms([&] {
        cublasGemmEx(h, CUBLAS_OP_N, CUBLAS_OP_N, N, M, K, &one, B, CUDA_R_16F, N, A, CUDA_R_16F, K, &zero, Cref,
                     CUDA_R_32F, N, CUBLAS_COMPUTE_32F, CUBLAS_GEMM_DEFAULT);
    });
    // ANCHOR_END: cublas
    CHECK(cudaGetLastError());

    std::vector<float> h1(size_t(M) * N), h2(h1.size()), hr(h1.size());
    CHECK(cudaMemcpy(h1.data(), C1, h1.size() * sizeof(float), cudaMemcpyDeviceToHost));
    CHECK(cudaMemcpy(h2.data(), C2, h1.size() * sizeof(float), cudaMemcpyDeviceToHost));
    CHECK(cudaMemcpy(hr.data(), Cref, h1.size() * sizeof(float), cudaMemcpyDeviceToHost));
    double d1 = 0, d2 = 0, ref = 0;
    for (size_t i = 0; i < hr.size(); ++i)
        d1 = fmax(d1, fabs(h1[i] - hr[i])), d2 = fmax(d2, fabs(h2[i] - hr[i])), ref = fmax(ref, fabs(hr[i]));
    printf("%d×%d×%d，half 输入、float 累加\n", M, N, K);
    printf("WMMA 每个 warp 一块，直接读显存 %7.2f ms  %5.1f TFLOPS  和 cuBLAS 的最大差 %.1e\n", t1, flop / t1 / 1e9, d1);
    printf("WMMA 经 shared memory 分块      %7.2f ms  %5.1f TFLOPS  和 cuBLAS 的最大差 %.1e\n", t2, flop / t2 / 1e9, d2);
    printf("cuBLAS（cublasGemmEx）          %7.2f ms  %5.1f TFLOPS  （结果最大绝对值 %.1f）\n", t3, flop / t3 / 1e9, ref);
}
