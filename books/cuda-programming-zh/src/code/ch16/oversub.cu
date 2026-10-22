// oversub.cu: managed 内存可以超过显存容量。数据放不下时，GPU 一边算一边和主机内存换页。
//   nvcc -O2 -arch=native oversub.cu -o oversub && ./oversub
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

__global__ void fill(float* p, size_t n) {
    for (size_t i = blockIdx.x * size_t(blockDim.x) + threadIdx.x; i < n; i += size_t(gridDim.x) * blockDim.x)
        p[i] = 1.0f;
}

__global__ void scale(float* p, size_t n, float k) {
    for (size_t i = blockIdx.x * size_t(blockDim.x) + threadIdx.x; i < n; i += size_t(gridDim.x) * blockDim.x)
        p[i] *= k;
}

double now_ms() {
    using namespace std::chrono;
    return duration<double, std::milli>(steady_clock::now().time_since_epoch()).count();
}

int main() {
    size_t free_b, total_b;
    CHECK(cudaMemGetInfo(&free_b, &total_b));
    printf("显存 %.1f GB，其中可用 %.1f GB\n\n", total_b / 1e9, free_b / 1e9);
    printf("数据量    第一遍读写       第二遍读写\n");
    for (double gb : {2.0, 4.0, 6.0, 7.0, 8.0, 9.0}) {
        size_t bytes = size_t(gb * (1 << 30));
        size_t n = bytes / sizeof(float);
        float* p;
        CHECK(cudaMallocManaged(&p, bytes));
        fill<<<26 * 6, 256>>>(p, n);           // 在 GPU 上初始化，页面一开始就在显存里（放得下的话）
        CHECK(cudaGetLastError());
        CHECK(cudaDeviceSynchronize());
        printf("%4.0f GB", gb);
        fflush(stdout);
        for (int pass = 0; pass < 2; ++pass) {
            double t0 = now_ms();
            scale<<<26 * 6, 256>>>(p, n, 1.0001f);
            CHECK(cudaGetLastError());
            CHECK(cudaDeviceSynchronize());
            double ms = now_ms() - t0;
            printf("  %8.0f ms %5.1f GB/s", ms, 2.0 * bytes / (ms / 1e3) / 1e9);
            fflush(stdout);
        }
        printf("\n");
        CHECK(cudaFree(p));
    }
}
