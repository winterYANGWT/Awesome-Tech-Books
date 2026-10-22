// dsm_read.cu: 成块读取的带宽：读自己 block 的 shared memory、读同一 cluster 里另一个 block 的
// shared memory（分布式 shared memory）、读 global memory（数据在 L2 里）。
//   nvcc -O2 -arch=native dsm_read.cu -o dsm_read && ./dsm_read
#include <cooperative_groups.h>
#include <cstdio>
#include <cstdlib>

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

constexpr int WORDS = 8192;     // 每个 block 32 KB 的 float4 数据（2048 个 float4）
constexpr int REPS = 2000;

// ANCHOR: kernel
// source：0 读自己的 shared memory，1 读 cluster 里下一个 block 的，2 读 global memory。
__global__ void __cluster_dims__(2, 1, 1) read_bw(const float4* global_src, int source, float* out) {
    __shared__ float4 tile[WORDS / 4];
    cg::cluster_group cluster = cg::this_cluster();
    for (int i = threadIdx.x; i < WORDS / 4; i += blockDim.x) tile[i] = make_float4(i, i, i, i);
    cluster.sync();

    const float4* src = tile;
    if (source == 1) src = cluster.map_shared_rank(tile, (cluster.block_rank() + 1) % cluster.num_blocks());
    if (source == 2) src = global_src + blockIdx.x * (WORDS / 4);
    float4 acc = make_float4(0, 0, 0, 0);
    for (int r = 0; r < REPS; ++r)
        for (int i = threadIdx.x; i < WORDS / 4; i += blockDim.x) {
            float4 v = src[i];
            acc.x += v.x, acc.y += v.y, acc.z += v.z, acc.w += v.w;
        }
    out[blockIdx.x * blockDim.x + threadIdx.x] = acc.x + acc.y + acc.z + acc.w;
    cluster.sync();                     // 别人还可能在读我的 shared memory，不能提前退出
}
// ANCHOR_END: kernel

int main() {
    int sms;
    CHECK(cudaDeviceGetAttribute(&sms, cudaDevAttrMultiProcessorCount, 0));
    const int blocks = sms * 2, threads = 512;
    float4* g;
    float* out;
    CHECK(cudaMalloc(&g, size_t(blocks) * WORDS * sizeof(float)));
    CHECK(cudaMemset(g, 0, size_t(blocks) * WORDS * sizeof(float)));
    CHECK(cudaMalloc(&out, blocks * threads * sizeof(float)));
    cudaEvent_t start, stop;
    CHECK(cudaEventCreate(&start));
    CHECK(cudaEventCreate(&stop));
    const char* names[] = {"自己的 shared memory", "cluster 里另一个 block 的 shared memory", "global memory（在 L2 里）"};
    for (int source = 0; source < 3; ++source) {
        read_bw<<<blocks, threads>>>(g, source, out);
        CHECK(cudaGetLastError());
        CHECK(cudaEventRecord(start));
        read_bw<<<blocks, threads>>>(g, source, out);
        CHECK(cudaEventRecord(stop));
        CHECK(cudaEventSynchronize(stop));
        float ms;
        CHECK(cudaEventElapsedTime(&ms, start, stop));
        double bytes = double(blocks) * WORDS * sizeof(float) * REPS;
        printf("%7.0f GB/s  %s\n", bytes / (ms / 1e3) / 1e9, names[source]);
    }
    CHECK(cudaFree(g));
    CHECK(cudaFree(out));
}
