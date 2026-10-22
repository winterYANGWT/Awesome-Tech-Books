// vecadd.cu: 第一个完整的 CUDA 程序，两个向量相加，并打印每一步花了多少时间。
//   nvcc -O2 -arch=native vecadd.cu -o vecadd && ./vecadd
// ANCHOR: check
#include <chrono>
#include <cmath>
#include <cstdio>
#include <cstdlib>

// 每个 CUDA API 都返回 cudaError_t，出错就打印位置和原因，然后退出。
#define CHECK(call)                                                        \
    do {                                                                   \
        cudaError_t err_ = (call);                                         \
        if (err_ != cudaSuccess) {                                         \
            fprintf(stderr, "%s:%d: %s (%s)\n", __FILE__, __LINE__,        \
                    cudaGetErrorName(err_), cudaGetErrorString(err_));     \
            exit(1);                                                       \
        }                                                                  \
    } while (0)
// ANCHOR_END: check

// ANCHOR: kernel
__global__ void add(const float* a, const float* b, float* c, int n) {
    int i = blockIdx.x * blockDim.x + threadIdx.x;
    if (i < n) c[i] = a[i] + b[i];
}
// ANCHOR_END: kernel

// 记录相邻两次 lap() 之间的时间。
struct Stopwatch {
    std::chrono::steady_clock::time_point last = std::chrono::steady_clock::now();
    void lap(const char* what) {
        auto now = std::chrono::steady_clock::now();
        printf("%9.3f ms  %s\n", std::chrono::duration<double, std::milli>(now - last).count(), what);
        last = now;
    }
};

int main() {
    const int n = 16 << 20;                 // 1600 多万个元素，每个数组 64 MiB
    const size_t bytes = n * sizeof(float);
    Stopwatch sw;

    // ANCHOR: host
    // 1. 在主机内存里准备输入。
    float* h_a = static_cast<float*>(malloc(bytes));
    float* h_b = static_cast<float*>(malloc(bytes));
    float* h_c = static_cast<float*>(malloc(bytes));
    if (!h_a || !h_b || !h_c) { fprintf(stderr, "malloc failed\n"); return 1; }
    for (int i = 0; i < n; ++i) h_a[i] = sinf(i) * sinf(i), h_b[i] = cosf(i) * cosf(i);
    sw.lap("准备主机数据");
    // ANCHOR_END: host

    // ANCHOR: device
    // 2. 在显存里分配三个数组。
    float *d_a, *d_b, *d_c;
    CHECK(cudaMalloc(&d_a, bytes));
    sw.lap("cudaMalloc d_a（程序里第一个 CUDA 调用）");
    CHECK(cudaMalloc(&d_b, bytes));
    CHECK(cudaMalloc(&d_c, bytes));
    sw.lap("cudaMalloc d_b、d_c");

    // 3. 把输入拷到显存。
    CHECK(cudaMemcpy(d_a, h_a, bytes, cudaMemcpyHostToDevice));
    CHECK(cudaMemcpy(d_b, h_b, bytes, cudaMemcpyHostToDevice));
    sw.lap("cudaMemcpy 两个输入，主机 -> 显存");
    // ANCHOR_END: device

    // ANCHOR: launch
    // 4. 启动 kernel：每个元素一个线程，256 个线程一个 block。
    int threads = 256;
    int blocks = (n + threads - 1) / threads;
    add<<<blocks, threads>>>(d_a, d_b, d_c, n);
    CHECK(cudaGetLastError());              // 启动本身有没有出错
    sw.lap("启动 kernel（第一次）");
    CHECK(cudaDeviceSynchronize());         // 等 kernel 跑完，执行中的错误在这里报告
    sw.lap("cudaDeviceSynchronize，等 kernel 跑完");
    // ANCHOR_END: launch

    add<<<blocks, threads>>>(d_a, d_b, d_c, n);
    CHECK(cudaGetLastError());
    sw.lap("再启动一次 kernel");
    CHECK(cudaDeviceSynchronize());
    sw.lap("cudaDeviceSynchronize");

    // ANCHOR: finish
    // 5. 把结果拷回来，检查。
    CHECK(cudaMemcpy(h_c, d_c, bytes, cudaMemcpyDeviceToHost));
    sw.lap("cudaMemcpy 结果，显存 -> 主机");
    int bad = 0;
    for (int i = 0; i < n; ++i)
        if (fabsf(h_c[i] - 1.0f) > 1e-5f) ++bad;   // sin² + cos² = 1
    printf("%d 个元素不等于 1\n", bad);

    // 6. 释放：显存用 cudaFree，主机内存用 free。
    CHECK(cudaFree(d_a));
    CHECK(cudaFree(d_b));
    CHECK(cudaFree(d_c));
    free(h_a);
    free(h_b);
    free(h_c);
    return bad ? 1 : 0;
    // ANCHOR_END: finish
}
