// wc.cu: write-combined pinned 内存：H2D 拷贝快不快，CPU 读它有多慢。
//   nvcc -O2 -arch=native wc.cu -o wc && ./wc
#include <chrono>
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

volatile double sink;

double now_ms() {
    using namespace std::chrono;
    return duration<double, std::milli>(steady_clock::now().time_since_epoch()).count();
}

int main() {
    const size_t n = size_t(16) << 20;       // 16M 个 float，64 MB
    float *normal, *wc, *dev;
    CHECK(cudaHostAlloc(&normal, n * sizeof(float), cudaHostAllocDefault));
    // ANCHOR: alloc
    CHECK(cudaHostAlloc(&wc, n * sizeof(float), cudaHostAllocWriteCombined));
    // ANCHOR_END: alloc
    CHECK(cudaMalloc(&dev, n * sizeof(float)));

    for (float* p : {normal, wc}) {
        double t0 = now_ms();
        for (size_t i = 0; i < n; ++i) p[i] = float(i);          // CPU 写
        double t1 = now_ms();
        double s = 0;
        for (size_t i = 0; i < n; ++i) s += p[i];                 // CPU 读
        double t2 = now_ms();
        sink = s;                                                 // 防止读循环被优化掉
        CHECK(cudaMemcpy(dev, p, n * sizeof(float), cudaMemcpyHostToDevice));   // 预热
        double t3 = now_ms();
        CHECK(cudaMemcpy(dev, p, n * sizeof(float), cudaMemcpyHostToDevice));
        double t4 = now_ms();
        printf("%-16s CPU 写 %5.1f ms   CPU 读 %6.1f ms   H2D 拷贝 %4.1f ms\n",
               p == wc ? "write-combined" : "普通 pinned", t1 - t0, t2 - t1, t4 - t3);
    }
    CHECK(cudaFreeHost(normal));
    CHECK(cudaFreeHost(wc));
    CHECK(cudaFree(dev));
}
