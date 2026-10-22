// gemm.cu: 4096×4096 的矩阵乘法 C = A·B（矩阵按 C++ 习惯行主序存放）。
// 比较自己写的 kernel 和 cuBLAS 的几种精度模式，并演示列主序带来的坑。
//   nvcc -O2 -arch=native gemm.cu -o gemm -lcublas && ./gemm
// NVCCFLAGS: -lcublas
#include <cublas_v2.h>
#include <cuda_bf16.h>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <random>
#include <vector>

#define CHECK(call)                                                        \
    do {                                                                   \
        cudaError_t err_ = (call);                                         \
        if (err_ != cudaSuccess) {                                         \
            fprintf(stderr, "%s:%d: %s (%s)\n", __FILE__, __LINE__,        \
                    cudaGetErrorName(err_), cudaGetErrorString(err_));     \
            exit(1);                                                       \
        }                                                                  \
    } while (0)

#define CHECK_BLAS(call)                                                   \
    do {                                                                   \
        cublasStatus_t st_ = (call);                                       \
        if (st_ != CUBLAS_STATUS_SUCCESS) {                                \
            fprintf(stderr, "%s:%d: cuBLAS 错误 %d\n", __FILE__, __LINE__,  \
                    int(st_));                                             \
            exit(1);                                                       \
        }                                                                  \
    } while (0)

constexpr int N = 4096;

// ANCHOR: naive
// 朴素写法：一个线程算 C 的一个元素。
__global__ void gemm_naive(const float* A, const float* B, float* C) {
    int col = blockIdx.x * blockDim.x + threadIdx.x, row = blockIdx.y * blockDim.y + threadIdx.y;
    float s = 0;
    for (int k = 0; k < N; ++k) s += A[row * N + k] * B[k * N + col];
    C[row * N + col] = s;
}
// ANCHOR_END: naive

// 分块写法：A、B 各取 32×32 的一块进 shared memory，块内反复使用（第 9 章的思路）。
__global__ void gemm_tiled(const float* A, const float* B, float* C) {
    __shared__ float As[32][33], Bs[32][33];
    int tx = threadIdx.x, ty = threadIdx.y;
    int col = blockIdx.x * 32 + tx, row = blockIdx.y * 32 + ty;
    float s = 0;
    for (int k0 = 0; k0 < N; k0 += 32) {
        As[ty][tx] = A[row * N + k0 + tx];
        Bs[ty][tx] = B[(k0 + ty) * N + col];
        __syncthreads();
        for (int k = 0; k < 32; ++k) s += As[ty][k] * Bs[k][tx];
        __syncthreads();
    }
    C[row * N + col] = s;
}

// ANCHOR: reg
// 寄存器分块：每个 block 算 C 的 64×64 一块，256 个线程每个算 4×4 个输出。
// 每从 shared memory 取 4 个 A 和 4 个 B，就能做 16 次乘加，而不是 1 次。
__global__ void gemm_reg(const float* A, const float* B, float* C) {
    __shared__ float As[16][64 + 4];          // A 的 64×16 块，转置存放：As[k][行]
    __shared__ float Bs[16][64];              // B 的 16×64 块
    int tx = threadIdx.x % 16, ty = threadIdx.x / 16;
    int row0 = blockIdx.y * 64, col0 = blockIdx.x * 64;
    float acc[4][4] = {};
    for (int k0 = 0; k0 < N; k0 += 16) {
        for (int i = threadIdx.x; i < 64 * 16; i += 256) {
            int r = i / 16, c = i % 16;
            As[c][r] = A[(row0 + r) * N + k0 + c];
        }
        for (int i = threadIdx.x; i < 16 * 64; i += 256) {
            int r = i / 64, c = i % 64;
            Bs[r][c] = B[(k0 + r) * N + col0 + c];
        }
        __syncthreads();
        for (int k = 0; k < 16; ++k) {
            float a[4], b[4];
            for (int i = 0; i < 4; ++i) a[i] = As[k][ty * 4 + i];
            for (int j = 0; j < 4; ++j) b[j] = Bs[k][tx * 4 + j];
            for (int i = 0; i < 4; ++i)
                for (int j = 0; j < 4; ++j) acc[i][j] += a[i] * b[j];
        }
        __syncthreads();
    }
    for (int i = 0; i < 4; ++i)
        for (int j = 0; j < 4; ++j) C[(row0 + ty * 4 + i) * N + col0 + tx * 4 + j] = acc[i][j];
}
// ANCHOR_END: reg

__global__ void to_bf16(const float* in, __nv_bfloat16* out, size_t n) {
    for (size_t i = blockIdx.x * size_t(blockDim.x) + threadIdx.x; i < n; i += size_t(gridDim.x) * blockDim.x)
        out[i] = __float2bfloat16(in[i]);
}

__global__ void to_double(const float* in, double* out, size_t n) {
    for (size_t i = blockIdx.x * size_t(blockDim.x) + threadIdx.x; i < n; i += size_t(gridDim.x) * blockDim.x)
        out[i] = in[i];
}

double now_ms() {
    using namespace std::chrono;
    return duration<double, std::milli>(steady_clock::now().time_since_epoch()).count();
}

int main() {
    const size_t nn = size_t(N) * N;
    std::vector<float> hA(nn), hB(nn), hC(nn);
    std::mt19937 rng(5);
    std::uniform_real_distribution<float> dist(-1.0f, 1.0f);
    for (auto& v : hA) v = dist(rng);
    for (auto& v : hB) v = dist(rng);
    float *A, *B, *C;
    CHECK(cudaMalloc(&A, nn * sizeof(float)));
    CHECK(cudaMalloc(&B, nn * sizeof(float)));
    CHECK(cudaMalloc(&C, nn * sizeof(float)));
    CHECK(cudaMemcpy(A, hA.data(), nn * sizeof(float), cudaMemcpyHostToDevice));
    CHECK(cudaMemcpy(B, hB.data(), nn * sizeof(float), cudaMemcpyHostToDevice));

    // ANCHOR: handle
    double t0 = now_ms();
    cublasHandle_t h;
    CHECK_BLAS(cublasCreate(&h));               // 创建 handle：初始化库、分配内部资源
    double t_create = now_ms() - t0;
    // ANCHOR_END: handle
    printf("cublasCreate 用了 %.1f ms\n\n", t_create);

    // 参照答案：用 double 在 GPU 上算一遍（行主序技巧见下文）。
    double *dA, *dB, *dC;
    CHECK(cudaMalloc(&dA, nn * sizeof(double)));
    CHECK(cudaMalloc(&dB, nn * sizeof(double)));
    CHECK(cudaMalloc(&dC, nn * sizeof(double)));
    to_double<<<1024, 256>>>(A, dA, nn);
    to_double<<<1024, 256>>>(B, dB, nn);
    const double one_d = 1.0, zero_d = 0.0;
    CHECK_BLAS(cublasDgemm(h, CUBLAS_OP_N, CUBLAS_OP_N, N, N, N, &one_d, dB, N, dA, N, &zero_d, dC, N));
    std::vector<double> ref(nn);
    CHECK(cudaMemcpy(ref.data(), dC, nn * sizeof(double), cudaMemcpyDeviceToHost));

    auto max_rel_err = [&] {
        CHECK(cudaMemcpy(hC.data(), C, nn * sizeof(float), cudaMemcpyDeviceToHost));
        double worst = 0;
        for (size_t i = 0; i < nn; i += 101) worst = std::fmax(worst, std::fabs(hC[i] - ref[i]) / (std::fabs(ref[i]) + 1.0));
        return worst;
    };
    cudaEvent_t start, stop;
    CHECK(cudaEventCreate(&start));
    CHECK(cudaEventCreate(&stop));
    auto run = [&](const char* label, auto launch) {
        launch();                               // 预热
        CHECK(cudaGetLastError());
        float best = 1e30f;
        for (int r = 0; r < 3; ++r) {
            CHECK(cudaEventRecord(start));
            launch();
            CHECK(cudaEventRecord(stop));
            CHECK(cudaEventSynchronize(stop));
            float ms;
            CHECK(cudaEventElapsedTime(&ms, start, stop));
            if (ms < best) best = ms;
        }
        printf("%8.2f ms  %6.2f TFLOPS  最大相对误差 %.1e  %s\n", best, 2.0 * N * N * N / (best / 1e3) / 1e12,
               max_rel_err(), label);
    };

    dim3 block(32, 32), grid(N / 32, N / 32);
    run("朴素 kernel", [&] { gemm_naive<<<grid, block>>>(A, B, C); });
    run("分块 kernel（shared memory）", [&] { gemm_tiled<<<grid, block>>>(A, B, C); });
    run("寄存器分块 kernel（每线程 4×4）", [&] { gemm_reg<<<dim3(N / 64, N / 64), 256>>>(A, B, C); });

    const float one = 1.0f, zero = 0.0f;
    // ANCHOR: sgemm
    // cuBLAS 按列主序理解矩阵。行主序的 A、B、C 在它看来是 Aᵀ、Bᵀ、Cᵀ，
    // 而 Cᵀ = Bᵀ·Aᵀ，所以把 B 放在前面、A 放在后面，就得到行主序的 C = A·B。
    run("cuBLAS SGEMM（FP32）", [&] {
        CHECK_BLAS(cublasSgemm(h, CUBLAS_OP_N, CUBLAS_OP_N, N, N, N, &one, B, N, A, N, &zero, C, N));
    });
    // ANCHOR_END: sgemm

    // ANCHOR: tf32
    CHECK_BLAS(cublasSetMathMode(h, CUBLAS_TF32_TENSOR_OP_MATH));   // 允许用张量核以 TF32 精度计算
    run("cuBLAS SGEMM（TF32 张量核）", [&] {
        CHECK_BLAS(cublasSgemm(h, CUBLAS_OP_N, CUBLAS_OP_N, N, N, N, &one, B, N, A, N, &zero, C, N));
    });
    CHECK_BLAS(cublasSetMathMode(h, CUBLAS_DEFAULT_MATH));
    // ANCHOR_END: tf32

    // ANCHOR: bf16
    // 输入转成 bfloat16，用张量核相乘，FP32 累加，结果仍是 float。
    __nv_bfloat16 *A16, *B16;
    CHECK(cudaMalloc(&A16, nn * sizeof(__nv_bfloat16)));
    CHECK(cudaMalloc(&B16, nn * sizeof(__nv_bfloat16)));
    to_bf16<<<1024, 256>>>(A, A16, nn);
    to_bf16<<<1024, 256>>>(B, B16, nn);
    run("cuBLAS GemmEx（BF16 输入，FP32 累加）", [&] {
        CHECK_BLAS(cublasGemmEx(h, CUBLAS_OP_N, CUBLAS_OP_N, N, N, N, &one, B16, CUDA_R_16BF, N, A16, CUDA_R_16BF, N,
                                &zero, C, CUDA_R_32F, N, CUBLAS_COMPUTE_32F, CUBLAS_GEMM_DEFAULT));
    });
    // ANCHOR_END: bf16

    // ANCHOR: wrong
    // 常见错误：照着数学公式的顺序把 A、B 传进去。cuBLAS 算出的是行主序下的 (B·A)，不是 A·B。
    run("cuBLAS SGEMM，参数顺序没有调换（错误）", [&] {
        CHECK_BLAS(cublasSgemm(h, CUBLAS_OP_N, CUBLAS_OP_N, N, N, N, &one, A, N, B, N, &zero, C, N));
    });
    // ANCHOR_END: wrong

    CHECK_BLAS(cublasDestroy(h));
    for (void* p : {(void*)A, (void*)B, (void*)C, (void*)dA, (void*)dB, (void*)dC, (void*)A16, (void*)B16})
        CHECK(cudaFree(p));
}
