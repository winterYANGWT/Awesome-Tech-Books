// cluster_hist.cu: 直方图的桶放在 cluster 里几个 block 的 shared memory 里（分布式 shared memory），
// 和直接在 global memory 上原子加比较。64K 个桶（256 KB）放不进一个 block 的 shared memory，
// 16K 个桶（64 KB）放得下，可以和只用自己 shared memory 的情况对照。
//   nvcc -O2 -arch=native cluster_hist.cu -o cluster_hist && ./cluster_hist
#include <cooperative_groups.h>
#include <cstdio>
#include <cstdlib>
#include <initializer_list>
#include <random>
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

// 做法一：直接在 global memory 上原子加。
template <int BINS>
__global__ void hist_global(const unsigned* keys, size_t n, unsigned* hist) {
    for (size_t i = blockIdx.x * size_t(blockDim.x) + threadIdx.x; i < n; i += size_t(gridDim.x) * blockDim.x)
        atomicAdd(&hist[keys[i]], 1u);
}

// ANCHOR: cluster
// 做法二：cluster 里的每个 block 持有 BINS / cluster 大小 个桶。
// 一个 key 归哪个 block 管，就加到那个 block 的 shared memory 里，可能是自己的，也可能是别人的。
template <int BINS>
__global__ void hist_cluster(const unsigned* keys, size_t n, unsigned* hist) {
    extern __shared__ unsigned part[];                 // 本 block 负责的那一段桶
    cg::cluster_group cluster = cg::this_cluster();
    const int per_block = BINS / cluster.num_blocks();
    for (int b = threadIdx.x; b < per_block; b += blockDim.x) part[b] = 0;
    cluster.sync();                                    // 所有 block 都清零之后才能互相加

    size_t stride = size_t(gridDim.x) * blockDim.x;
    for (size_t i = blockIdx.x * size_t(blockDim.x) + threadIdx.x; i < n; i += stride) {
        unsigned k = keys[i];
        unsigned owner = k / per_block;                // 哪个 block 管这个桶
        unsigned* remote = cluster.map_shared_rank(part, owner);   // 那个 block 的 shared memory
        atomicAdd(&remote[k % per_block], 1u);
    }
    cluster.sync();                                    // 所有人都加完，再把自己那段写回 global

    unsigned base = cluster.block_rank() * per_block;
    for (int b = threadIdx.x; b < per_block; b += blockDim.x)
        if (part[b]) atomicAdd(&hist[base + b], part[b]);
}
// ANCHOR_END: cluster

template <int BINS>
void experiment(bool skewed, std::initializer_list<int> cluster_sizes) {
    const size_t n = size_t(64) << 20;
    std::vector<unsigned> keys(n);
    std::mt19937 rng(1);
    // 均匀：key 均匀分布在所有桶上；倾斜：九成的 key 落在 64 个热门桶上。
    for (auto& k : keys) k = (skewed && rng() % 10 != 0) ? (rng() % 64) * (BINS / 64) : rng() % BINS;
    printf("%d 个桶，%s分布\n", BINS, skewed ? "倾斜" : "均匀");
    std::vector<unsigned> ref(BINS, 0), got(BINS);
    for (unsigned k : keys) ++ref[k];

    unsigned *d_keys, *d_hist;
    CHECK(cudaMalloc(&d_keys, n * sizeof(unsigned)));
    CHECK(cudaMalloc(&d_hist, BINS * sizeof(unsigned)));
    CHECK(cudaMemcpy(d_keys, keys.data(), n * sizeof(unsigned), cudaMemcpyHostToDevice));
    cudaEvent_t start, stop;
    CHECK(cudaEventCreate(&start));
    CHECK(cudaEventCreate(&stop));

    auto run = [&](const char* label, auto launch) {
        float best = 1e30f;
        for (int r = 0; r < 4; ++r) {
            CHECK(cudaMemset(d_hist, 0, BINS * sizeof(unsigned)));
            CHECK(cudaEventRecord(start));
            launch();
            CHECK(cudaEventRecord(stop));
            CHECK(cudaEventSynchronize(stop));
            CHECK(cudaGetLastError());
            float ms;
            CHECK(cudaEventElapsedTime(&ms, start, stop));
            if (r > 0 && ms < best) best = ms;
        }
        CHECK(cudaMemcpy(got.data(), d_hist, BINS * sizeof(unsigned), cudaMemcpyDeviceToHost));
        int bad = 0;
        for (int b = 0; b < BINS; ++b) bad += got[b] != ref[b];
        printf("%7.2f ms  %s%s\n", best, label, bad ? "  结果错误" : "");
    };

    run("global memory 原子加", [&] { hist_global<BINS><<<26 * 6, 256>>>(d_keys, n, d_hist); });

    CHECK(cudaFuncSetAttribute(hist_cluster<BINS>, cudaFuncAttributeMaxDynamicSharedMemorySize, 64 * 1024));
    for (int cs : cluster_sizes) {
        // ANCHOR: launch
        size_t smem = BINS / cs * sizeof(unsigned);    // 每个 block 的 shared memory
        cudaLaunchConfig_t cfg = {};
        cfg.blockDim = 1024;
        cfg.dynamicSmemBytes = smem;
        cudaLaunchAttribute attr = {};
        attr.id = cudaLaunchAttributeClusterDimension;  // 运行时指定 cluster 的大小
        attr.val.clusterDim.x = cs;
        attr.val.clusterDim.y = 1;
        attr.val.clusterDim.z = 1;
        cfg.attrs = &attr;
        cfg.numAttrs = 1;
        int clusters;
        cfg.gridDim = cs;
        CHECK(cudaOccupancyMaxActiveClusters(&clusters, hist_cluster<BINS>, &cfg));
        cfg.gridDim = clusters * cs;                   // 正好填满能同时驻留的 cluster
        // ANCHOR_END: launch
        char label[160];
        snprintf(label, sizeof(label), "cluster：%d 个 block 一组，每个 block %zu KB，共 %d 组", cs, smem >> 10, clusters);
        run(label, [&] { CHECK(cudaLaunchKernelEx(&cfg, hist_cluster<BINS>, (const unsigned*)d_keys, n, d_hist)); });
    }
    printf("\n");
    CHECK(cudaFree(d_keys));
    CHECK(cudaFree(d_hist));
}

int main() {
    experiment<1 << 16>(false, {4, 8});
    experiment<1 << 16>(true, {4, 8});
    experiment<1 << 14>(false, {1, 2, 4});
}
