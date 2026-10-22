// fir.cu: 64 抽头的 FIR 滤波 y[i] = Σ h[t]·x[i+t]。每个 block 一块一块地处理输出，
// 比较“直接读 global”“同步地先读后算”“用 memcpy_async 两级流水线”三种写法。
//   nvcc -O2 -arch=native fir.cu -o fir && ./fir
#include <cooperative_groups.h>
#include <cuda/barrier>
#include <cuda/pipeline>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <vector>

namespace cg = cooperative_groups;
using block_barrier = cuda::barrier<cuda::thread_scope_block>;

#define CHECK(call)                                                        \
    do {                                                                   \
        cudaError_t err_ = (call);                                         \
        if (err_ != cudaSuccess) {                                         \
            fprintf(stderr, "%s:%d: %s (%s)\n", __FILE__, __LINE__,        \
                    cudaGetErrorName(err_), cudaGetErrorString(err_));     \
            exit(1);                                                       \
        }                                                                  \
    } while (0)

constexpr int TAPS = 64;
constexpr int TILE = 1024;               // 每块 1024 个输出
constexpr int SPAN = TILE + TAPS;        // 需要读入的输入个数（多读几个，凑成 16 字节的整数倍）
constexpr int THREADS = 256;             // 每个线程算 4 个输出
__constant__ float h[TAPS];

// 算一块：每个线程负责 4 个相邻的输出。xs 指向这一块输入的开头。
__device__ void compute_tile(const float* xs, float* y_tile) {
    int j = threadIdx.x * 4;
    float a0 = 0, a1 = 0, a2 = 0, a3 = 0;
#pragma unroll 16
    for (int t = 0; t < TAPS; ++t) {
        float c = h[t];
        a0 += c * xs[j + t], a1 += c * xs[j + 1 + t], a2 += c * xs[j + 2 + t], a3 += c * xs[j + 3 + t];
    }
    reinterpret_cast<float4*>(y_tile)[threadIdx.x] = make_float4(a0, a1, a2, a3);
}

// 做法一：不用 shared memory，直接从 global memory 读，靠 L1 缓存复用。
__global__ void fir_global(const float* x, float* y, int tiles) {
    extern __shared__ char unused[];         // 动态 shared memory 只用来占位，限制每个 SM 的 block 数
    for (int tile = blockIdx.x; tile < tiles; tile += gridDim.x)
        compute_tile(x + size_t(tile) * TILE, y + size_t(tile) * TILE);
}

// ANCHOR: sync
// 做法二：先把整块输入读进 shared memory，同步，再算。读的时候不能算，算的时候不读。
__global__ void fir_sync(const float* x, float* y, int tiles) {
    extern __shared__ char unused[];
    __shared__ __align__(16) float xs[SPAN];
    for (int tile = blockIdx.x; tile < tiles; tile += gridDim.x) {
        const float4* src = reinterpret_cast<const float4*>(x + size_t(tile) * TILE);
        for (int i = threadIdx.x; i < SPAN / 4; i += blockDim.x) reinterpret_cast<float4*>(xs)[i] = src[i];
        __syncthreads();
        compute_tile(xs, y + size_t(tile) * TILE);
        __syncthreads();                     // 大家都算完，才能覆盖 xs
    }
}
// ANCHOR_END: sync

// ANCHOR: pipeline
// 做法三：两块 shared memory 轮流用。算第 k 块的同时，第 k+1 块已经在异步地往另一块里搬。
__global__ void fir_pipeline(const float* x, float* y, int tiles) {
    extern __shared__ char unused[];
    __shared__ __align__(16) float xs[2][SPAN];
    cuda::pipeline<cuda::thread_scope_thread> pipe = cuda::make_pipeline();   // 每个线程自己的流水线

    // 发起一块的异步拷贝：每个线程搬它负责的那几个 float4，global → shared，不经过寄存器。
    auto load = [&](int tile, int buf) {
        pipe.producer_acquire();
        if (tile < tiles) {
            const float4* src = reinterpret_cast<const float4*>(x + size_t(tile) * TILE);
            float4* dst = reinterpret_cast<float4*>(xs[buf]);
            for (int i = threadIdx.x; i < SPAN / 4; i += blockDim.x)
                cuda::memcpy_async(&dst[i], &src[i], sizeof(float4), pipe);
        }
        pipe.producer_commit();              // 这一批拷贝作为流水线的一级
    };

    int buf = 0;
    load(blockIdx.x, buf);                   // 先把第一块发出去
    for (int tile = blockIdx.x; tile < tiles; tile += gridDim.x) {
        load(tile + gridDim.x, buf ^ 1);     // 下一块开始往另一块 shared memory 里搬
        pipe.consumer_wait();                // 等当前这一块搬完（只等最早的那一级）
        __syncthreads();                     // 别的线程搬的部分也要到齐
        compute_tile(xs[buf], y + size_t(tile) * TILE);
        __syncthreads();                     // 大家都算完，这块 shared memory 才能被下一轮覆盖
        pipe.consumer_release();
        buf ^= 1;
    }
}
// ANCHOR_END: pipeline

// ANCHOR: barrier
// 做法四：同样的双缓冲，完成通知交给 barrier。整个 block 合作发起一块的拷贝，
// 拷贝完成和“所有线程都到了”这两件事都由这块 shared memory 对应的 barrier 跟踪。
__global__ void fir_barrier(const float* x, float* y, int tiles) {
    extern __shared__ char unused[];
    __shared__ __align__(16) float xs[2][SPAN];
    // barrier 有构造函数，而 __shared__ 变量不能动态初始化：先放一块对齐的原始存储，再用 init() 初始化。
    __shared__ alignas(block_barrier) unsigned char bar_storage[2 * sizeof(block_barrier)];
    block_barrier* bar = reinterpret_cast<block_barrier*>(bar_storage);
    cg::thread_block block = cg::this_thread_block();
    if (block.thread_rank() == 0) {
        init(&bar[0], block.size());         // 每一轮要等 block 里所有线程到达
        init(&bar[1], block.size());
    }
    block.sync();

    auto load = [&](int tile, int buf) {
        if (tile < tiles)
            cuda::memcpy_async(block, xs[buf], x + size_t(tile) * TILE,
                               cuda::aligned_size_t<16>(SPAN * sizeof(float)), bar[buf]);
    };
    int buf = 0;
    load(blockIdx.x, buf);
    for (int tile = blockIdx.x; tile < tiles; tile += gridDim.x) {
        load(tile + gridDim.x, buf ^ 1);     // 下一块往另一块 shared memory 里搬
        bar[buf].arrive_and_wait();          // 当前这块：拷贝完成、所有线程到达，才往下走
        compute_tile(xs[buf], y + size_t(tile) * TILE);
        block.sync();                        // 都算完，这块才能在下一轮被覆盖
        buf ^= 1;
    }
}
// ANCHOR_END: barrier

int main() {
    const int tiles = 1 << 15;               // 3200 多万个输出
    const size_t n_out = size_t(tiles) * TILE, n_in = n_out + TAPS;
    std::vector<float> hx(n_in), hh(TAPS), hy(n_out);
    for (size_t i = 0; i < n_in; ++i) hx[i] = std::sin(0.001f * i);
    for (int t = 0; t < TAPS; ++t) hh[t] = 1.0f / TAPS;
    CHECK(cudaMemcpyToSymbol(h, hh.data(), TAPS * sizeof(float)));
    float *x, *y;
    CHECK(cudaMalloc(&x, n_in * sizeof(float)));
    CHECK(cudaMalloc(&y, n_out * sizeof(float)));
    CHECK(cudaMemcpy(x, hx.data(), n_in * sizeof(float), cudaMemcpyHostToDevice));
    int sms;
    CHECK(cudaDeviceGetAttribute(&sms, cudaDevAttrMultiProcessorCount, 0));
    cudaEvent_t start, stop;
    CHECK(cudaEventCreate(&start));
    CHECK(cudaEventCreate(&stop));

    // per_sm：用动态 shared memory 占位，把每个 SM 驻留的 block 数限制在这个数以内。
    auto run = [&](const char* label, auto kernel, int per_sm) {
        CHECK(cudaFuncSetAttribute(kernel, cudaFuncAttributeMaxDynamicSharedMemorySize, 90 * 1024));
        size_t smem = 0;
        int actual;
        for (;; smem += 1024) {
            CHECK(cudaOccupancyMaxActiveBlocksPerMultiprocessor(&actual, kernel, THREADS, smem));
            if (actual <= per_sm) break;
        }
        int grid = sms * actual;
        float best = 1e30f;
        for (int r = 0; r < 4; ++r) {
            CHECK(cudaEventRecord(start));
            kernel<<<grid, THREADS, smem>>>(x, y, tiles);
            CHECK(cudaEventRecord(stop));
            CHECK(cudaEventSynchronize(stop));
            CHECK(cudaGetLastError());
            float ms;
            CHECK(cudaEventElapsedTime(&ms, start, stop));
            if (r > 0 && ms < best) best = ms;
        }
        CHECK(cudaMemcpy(hy.data(), y, n_out * sizeof(float), cudaMemcpyDeviceToHost));
        size_t bad = 0;
        for (size_t i = 0; i < n_out; i += 997) {
            float ref = 0;
            for (int t = 0; t < TAPS; ++t) ref += hh[t] * hx[i + t];
            bad += std::fabs(ref - hy[i]) > 1e-4f;
        }
        double gflops = 2.0 * n_out * TAPS / (best / 1e3) / 1e9;
        printf("%7.3f ms  %6.0f GFLOPS  %s%s\n", best, gflops, label, bad ? "  结果错误" : "");
    };
    for (int per_sm : {6, 2, 1}) {
        printf("每个 SM 最多 %d 个 block：\n", per_sm);
        run("直接读 global", fir_global, per_sm);
        run("同步：先读后算", fir_sync, per_sm);
        run("memcpy_async 两级流水线", fir_pipeline, per_sm);
        run("memcpy_async + barrier", fir_barrier, per_sm);
        printf("\n");
    }
    CHECK(cudaFree(x));
    CHECK(cudaFree(y));
}
