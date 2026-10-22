// cg_reduce.cu: 用 Cooperative Groups 重写第 11 章的归约，和手写 shuffle 的版本比较。
//   nvcc -O2 -arch=native cg_reduce.cu -o cg_reduce && ./cg_reduce
#include <cooperative_groups.h>
#include <cooperative_groups/reduce.h>
#include <cstdio>
#include <cstdlib>
#include <vector>

namespace cg = cooperative_groups;

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

// 第 11 章版本 4：手写 shuffle。
__device__ float warp_sum(float v) {
    for (int offset = 16; offset > 0; offset /= 2) v += __shfl_down_sync(0xffffffff, v, offset);
    return v;
}

__global__ void reduce_shuffle(const float* in, int n, float* out) {
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

// ANCHOR: cg
// 同样的算法，用 Cooperative Groups 表达：线程组是对象，组内的同步和归约是组的操作。
__global__ void reduce_cg(const float* in, int n, float* out) {
    __shared__ float warp_sums[THREADS / 32];
    cg::thread_block block = cg::this_thread_block();
    cg::thread_block_tile<32> warp = cg::tiled_partition<32>(block);   // 把 block 切成 32 个线程一组
    cg::grid_group grid = cg::this_grid();

    float sum = 0.0f;
    for (unsigned long long i = grid.thread_rank(); i < n; i += grid.num_threads()) sum += in[i];
    sum = cg::reduce(warp, sum, cg::plus<float>());                   // warp 内求和
    if (warp.thread_rank() == 0) warp_sums[warp.meta_group_rank()] = sum;
    block.sync();                                                      // 等价于 __syncthreads()
    if (warp.meta_group_rank() == 0) {
        sum = warp.thread_rank() < warp.meta_group_size() ? warp_sums[warp.thread_rank()] : 0.0f;
        sum = cg::reduce(warp, sum, cg::plus<float>());
        if (warp.thread_rank() == 0) atomicAdd(out, sum);
    }
}
// ANCHOR_END: cg

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
        printf("%-16s %6.3f ms  和 = %.6e\n", label, best, result);
    };
    run("手写 shuffle", reduce_shuffle);
    run("Cooperative Groups", reduce_cg);
    CHECK(cudaFree(in));
    CHECK(cudaFree(out));
}
