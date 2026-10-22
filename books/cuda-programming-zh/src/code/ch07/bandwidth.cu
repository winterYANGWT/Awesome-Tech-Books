// bandwidth.cu: 拷贝 kernel 在不同数据量下的带宽。数据放得进 L2 时，反复拷贝走的是 L2。
//   nvcc -O2 -arch=native bandwidth.cu -o bandwidth && ./bandwidth
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

__global__ void copy(const float4* in, float4* out, size_t n) {
    size_t stride = size_t(gridDim.x) * blockDim.x;
    for (size_t i = blockIdx.x * size_t(blockDim.x) + threadIdx.x; i < n; i += stride)
        out[i] = in[i];
}

int main() {
    const size_t max_bytes = size_t(256) << 20;
    float4 *in, *out;
    CHECK(cudaMalloc(&in, max_bytes));
    CHECK(cudaMalloc(&out, max_bytes));
    CHECK(cudaMemset(in, 0, max_bytes));
    cudaEvent_t start, stop;
    CHECK(cudaEventCreate(&start));
    CHECK(cudaEventCreate(&stop));

    printf("每个数组      带宽\n");
    for (size_t bytes = size_t(1) << 20; bytes <= max_bytes; bytes *= 2) {
        size_t n = bytes / sizeof(float4);
        int reps = int(max_bytes / bytes);            // 每次一共搬 256 MiB × 2
        copy<<<26 * 6, 256>>>(in, out, n);             // 预热，同时把数据带进 L2
        CHECK(cudaGetLastError());
        CHECK(cudaEventRecord(start));
        for (int r = 0; r < reps; ++r) copy<<<26 * 6, 256>>>(in, out, n);
        CHECK(cudaEventRecord(stop));
        CHECK(cudaEventSynchronize(stop));
        float ms;
        CHECK(cudaEventElapsedTime(&ms, start, stop));
        printf("%4zu MB  %7.0f GB/s\n", bytes >> 20, 2.0 * bytes * reps / (ms / 1e3) / 1e9);
    }
    CHECK(cudaFree(in));
    CHECK(cudaFree(out));
}
