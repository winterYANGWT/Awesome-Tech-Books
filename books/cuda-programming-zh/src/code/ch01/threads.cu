// threads.cu: 同一个拷贝任务，只改同时在跑的线程数，看显存带宽怎么变。
//   nvcc -O2 -arch=native threads.cu -o threads && ./threads
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

// ANCHOR: copy
// 总线程数不够 n 个时，每个线程隔 stride 个元素处理一个，直到做完。
__global__ void copy(const float* in, float* out, size_t n) {
    size_t stride = size_t(gridDim.x) * blockDim.x;
    for (size_t i = blockIdx.x * size_t(blockDim.x) + threadIdx.x; i < n; i += stride)
        out[i] = in[i];
}
// ANCHOR_END: copy

int main() {
    const size_t n = size_t(64) << 20;   // 256 MiB
    float *in, *out;
    CHECK(cudaMalloc(&in, n * sizeof(float)));
    CHECK(cudaMalloc(&out, n * sizeof(float)));
    CHECK(cudaMemset(in, 0, n * sizeof(float)));
    cudaEvent_t start, stop;
    CHECK(cudaEventCreate(&start));
    CHECK(cudaEventCreate(&stop));

    copy<<<1024, 256>>>(in, out, n);   // 预热
    CHECK(cudaDeviceSynchronize());

    printf("   线程总数    带宽\n");
    for (int total = 32; total <= (1 << 20); total *= 2) {
        int threads = total < 256 ? total : 256;
        int blocks = total / threads;
        float best = 1e30f;
        for (int r = 0; r < 3; ++r) {
            CHECK(cudaEventRecord(start));
            copy<<<blocks, threads>>>(in, out, n);
            CHECK(cudaEventRecord(stop));
            CHECK(cudaEventSynchronize(stop));
            float ms;
            CHECK(cudaEventElapsedTime(&ms, start, stop));
            if (ms < best) best = ms;
        }
        CHECK(cudaGetLastError());
        // 读一遍、写一遍，一共搬了 2n 个 float。
        printf("%10d  %7.1f GB/s\n", total, 2.0 * n * sizeof(float) / (best / 1e3) / 1e9);
    }
    CHECK(cudaFree(in));
    CHECK(cudaFree(out));
}
