// block_reduce.cu: 在自己的 kernel 里用 CUB 的 block 级归约，和第 11 章手写的版本比较。
//   nvcc -O2 -arch=native block_reduce.cu -o block_reduce && ./block_reduce
#include <cub/cub.cuh>
#include <cstdio>
#include <cstdlib>
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

constexpr int THREADS = 256;

// 第 11 章版本 4：手写 warp shuffle + shared memory。
__device__ float warp_sum(float v) {
    for (int offset = 16; offset > 0; offset /= 2) v += __shfl_down_sync(0xffffffff, v, offset);
    return v;
}

__global__ void reduce_hand(const float* in, int n, float* out) {
    __shared__ float warp_sums[THREADS / 32];
    int t = threadIdx.x, lane = t % 32, warp = t / 32;
    float sum = 0.0f;
    for (int i = blockIdx.x * blockDim.x + t; i < n; i += gridDim.x * blockDim.x) sum += in[i];
    sum = warp_sum(sum);
    if (lane == 0) warp_sums[warp] = sum;
    __syncthreads();
    if (warp == 0) {
        sum = lane < THREADS / 32 ? warp_sums[lane] : 0.0f;
        sum = warp_sum(sum);
        if (lane == 0) atomicAdd(out, sum);
    }
}

// ANCHOR: cub
// 同样的结构，block 内的归约交给 cub::BlockReduce。
__global__ void reduce_cub(const float* in, int n, float* out) {
    using BlockReduce = cub::BlockReduce<float, THREADS>;
    __shared__ typename BlockReduce::TempStorage temp;   // CUB 需要的 shared memory，由调用者提供
    float sum = 0.0f;
    for (int i = blockIdx.x * blockDim.x + threadIdx.x; i < n; i += gridDim.x * blockDim.x) sum += in[i];
    float block_sum = BlockReduce(temp).Sum(sum);        // 结果只在 0 号线程里有效
    if (threadIdx.x == 0) atomicAdd(out, block_sum);
}
// ANCHOR_END: cub

int main() {
    const int n = 64 << 20;
    std::vector<float> h(n);
    for (int i = 0; i < n; ++i) h[i] = float((i * 7919LL) % 1000) / 1000.0f;
    float *in, *out;
    CHECK(cudaMalloc(&in, n * sizeof(float)));
    CHECK(cudaMalloc(&out, sizeof(float)));
    CHECK(cudaMemcpy(in, h.data(), n * sizeof(float), cudaMemcpyHostToDevice));
    cudaEvent_t start, stop;
    CHECK(cudaEventCreate(&start));
    CHECK(cudaEventCreate(&stop));
    auto run = [&](const char* label, auto kernel) {
        float best = 1e30f, result = 0;
        for (int r = 0; r < 4; ++r) {
            CHECK(cudaMemset(out, 0, sizeof(float)));
            CHECK(cudaEventRecord(start));
            kernel<<<26 * 6, THREADS>>>(in, n, out);
            CHECK(cudaEventRecord(stop));
            CHECK(cudaEventSynchronize(stop));
            CHECK(cudaGetLastError());
            float ms;
            CHECK(cudaEventElapsedTime(&ms, start, stop));
            if (r > 0 && ms < best) best = ms;
            CHECK(cudaMemcpy(&result, out, sizeof(float), cudaMemcpyDeviceToHost));
        }
        printf("%6.3f ms  和 = %.6e  %s\n", best, result, label);
    };
    run("手写 shuffle", reduce_hand);
    run("cub::BlockReduce", reduce_cub);
    CHECK(cudaFree(in));
    CHECK(cudaFree(out));
}
