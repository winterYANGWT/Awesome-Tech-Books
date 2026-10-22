// loops.cu: 一轻一重两个循环，分别在 CPU 单线程、CPU 全部线程和 GPU 上计时。
//   nvcc -O2 -arch=native -Xcompiler "-O3 -march=native -fopenmp" loops.cu -o loops && ./loops
// NVCCFLAGS: -Xcompiler "-O3 -march=native -fopenmp"
#include <chrono>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <omp.h>
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

constexpr int N = 64 << 20;      // 64M 个元素
constexpr int HEAVY_ITERS = 256;  // 重循环里每个元素迭代的次数

// ANCHOR: loops
// #pragma omp simd 让编译器用 AVX-512 一次算 16 个元素，对 CPU 公平一些。
// 轻：每个元素一次乘加，读 8 字节、写 4 字节。
void saxpy_cpu(int n, float a, const float* x, float* y) {
#pragma omp simd
    for (int i = 0; i < n; ++i) y[i] = a * x[i] + y[i];
}

// 重：每个元素 256 次迭代，约 512 次浮点运算，读写各 4 字节。
__host__ __device__ inline float heavy(float v) {
    for (int k = 0; k < HEAVY_ITERS; ++k) v = 0.5f * v * v + 0.25f;
    return v;
}

void heavy_cpu(int n, const float* x, float* y) {
#pragma omp simd
    for (int i = 0; i < n; ++i) y[i] = heavy(x[i]);
}
// ANCHOR_END: loops

// 同样的循环，用 OpenMP 分给所有 CPU 线程，每个线程内部照样用 SIMD。
void saxpy_omp(int n, float a, const float* x, float* y) {
#pragma omp parallel for simd
    for (int i = 0; i < n; ++i) y[i] = a * x[i] + y[i];
}

void heavy_omp(int n, const float* x, float* y) {
#pragma omp parallel for simd
    for (int i = 0; i < n; ++i) y[i] = heavy(x[i]);
}

// ANCHOR: kernels
// GPU 版本：循环没有了，每个线程处理一个 i。
__global__ void saxpy_gpu(int n, float a, const float* x, float* y) {
    int i = blockIdx.x * blockDim.x + threadIdx.x;
    if (i < n) y[i] = a * x[i] + y[i];
}

__global__ void heavy_gpu(int n, const float* x, float* y) {
    int i = blockIdx.x * blockDim.x + threadIdx.x;
    if (i < n) y[i] = heavy(x[i]);
}
// ANCHOR_END: kernels

double now_ms() {
    using namespace std::chrono;
    return duration<double, std::milli>(steady_clock::now().time_since_epoch()).count();
}

template <class F>
double best_of(int reps, F f) {   // 跑几次取最快的一次
    double t = 1e30;
    for (int r = 0; r < reps; ++r) {
        double t0 = now_ms();
        f();
        t = fmin(t, now_ms() - t0);
    }
    return t;
}

int main() {
    size_t bytes = size_t(N) * sizeof(float);
    std::vector<float> x(N), y(N), out(N);
    for (int i = 0; i < N; ++i) x[i] = float(i % 1000) / 1000.0f, y[i] = 1.0f;

    float *d_x, *d_y;
    CHECK(cudaMalloc(&d_x, bytes));
    CHECK(cudaMalloc(&d_y, bytes));
    CHECK(cudaMemcpy(d_x, x.data(), bytes, cudaMemcpyHostToDevice));
    CHECK(cudaMemcpy(d_y, y.data(), bytes, cudaMemcpyHostToDevice));
    int threads = 256, blocks = (N + threads - 1) / threads;

    printf("%d 个 CPU 线程\n\n", omp_get_max_threads());
    printf("                       saxpy        heavy\n");

    double s1 = best_of(3, [&] { saxpy_cpu(N, 2.0f, x.data(), y.data()); });
    double h1 = best_of(1, [&] { heavy_cpu(N, x.data(), out.data()); });
    printf("CPU 单线程         %8.1f ms  %8.1f ms\n", s1, h1);

    double sn = best_of(5, [&] { saxpy_omp(N, 2.0f, x.data(), y.data()); });
    double hn = best_of(3, [&] { heavy_omp(N, x.data(), out.data()); });
    printf("CPU 全部线程       %8.1f ms  %8.1f ms\n", sn, hn);

    // 只算 kernel：数据已经在显存里。
    auto sync = [] { CHECK(cudaDeviceSynchronize()); };
    saxpy_gpu<<<blocks, threads>>>(N, 2.0f, d_x, d_y);   // 预热
    heavy_gpu<<<blocks, threads>>>(N, d_x, d_y);
    sync();
    double sg = best_of(5, [&] { saxpy_gpu<<<blocks, threads>>>(N, 2.0f, d_x, d_y); sync(); });
    double hg = best_of(5, [&] { heavy_gpu<<<blocks, threads>>>(N, d_x, d_y); sync(); });
    printf("GPU，只算 kernel   %8.1f ms  %8.1f ms\n", sg, hg);

    // 算上拷贝：输入拷上去，结果拷回来。
    double sc = best_of(3, [&] {
        CHECK(cudaMemcpy(d_x, x.data(), bytes, cudaMemcpyHostToDevice));
        CHECK(cudaMemcpy(d_y, y.data(), bytes, cudaMemcpyHostToDevice));
        saxpy_gpu<<<blocks, threads>>>(N, 2.0f, d_x, d_y);
        CHECK(cudaMemcpy(y.data(), d_y, bytes, cudaMemcpyDeviceToHost));
    });
    double hc = best_of(3, [&] {
        CHECK(cudaMemcpy(d_x, x.data(), bytes, cudaMemcpyHostToDevice));
        heavy_gpu<<<blocks, threads>>>(N, d_x, d_y);
        CHECK(cudaMemcpy(out.data(), d_y, bytes, cudaMemcpyDeviceToHost));
    });
    printf("GPU，算上拷贝      %8.1f ms  %8.1f ms\n", sc, hc);

    // 抽查：GPU 的 heavy 结果和 CPU 一致。
    std::vector<float> ref(N);
    heavy_omp(N, x.data(), ref.data());
    for (int i = 0; i < N; i += 9973)
        if (fabsf(ref[i] - out[i]) > 1e-5f) { printf("结果不一致 i=%d\n", i); return 1; }

    CHECK(cudaFree(d_x));
    CHECK(cudaFree(d_y));
}
