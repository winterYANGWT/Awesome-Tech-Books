// atomics.cu: 6400 万次原子加，地址的争用程度不同、作用域不同，速度差多少；
// 以及 CPU 和 GPU 同时给一个计数器加 1，作用域选错会怎样。
//   nvcc -O2 -arch=native atomics.cu -o atomics && ./atomics
#include <cuda/atomic>
#include <cooperative_groups.h>
#include <cooperative_groups/reduce.h>
#include <cstdio>
#include <cstdlib>
#include <thread>

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

constexpr int PER_THREAD = 64;
constexpr int BLOCKS = 26 * 6 * 26, THREADS = 256;    // 一共 6400 多万次加法

// ANCHOR: contention
// 所有线程都加到同一个地址上。
__global__ void one_address(int* c) {
    for (int k = 0; k < PER_THREAD; ++k) atomicAdd(c, 1);
}

// 每个 warp 一个地址：同一个 warp 的 32 个线程争同一个地址。
__global__ void per_warp(int* c) {
    int w = (blockIdx.x * blockDim.x + threadIdx.x) / 32;
    for (int k = 0; k < PER_THREAD; ++k) atomicAdd(&c[w], 1);
}

// 每个线程一个地址，没有争用。
__global__ void per_thread(int* c) {
    int t = blockIdx.x * blockDim.x + threadIdx.x;
    for (int k = 0; k < PER_THREAD; ++k) atomicAdd(&c[t], 1);
}

// 先在 warp 内把要加的数合起来，每个 warp 只做一次原子加。
__global__ void warp_aggregated(int* c) {
    cg::thread_block_tile<32> warp = cg::tiled_partition<32>(cg::this_thread_block());
    for (int k = 0; k < PER_THREAD; ++k) {
        int sum = cg::reduce(warp, 1, cg::plus<int>());
        if (warp.thread_rank() == 0) atomicAdd(c, sum);
    }
}
// ANCHOR_END: contention

// ANCHOR: scope
// 每个 block 一个计数器，block 内的线程争它。三种作用域：block、device、system。
__global__ void scope_block(int* c) {
    for (int k = 0; k < PER_THREAD; ++k) atomicAdd_block(&c[blockIdx.x], 1);
}
__global__ void scope_device(int* c) {
    for (int k = 0; k < PER_THREAD; ++k) atomicAdd(&c[blockIdx.x], 1);
}
__global__ void scope_system(int* c) {
    for (int k = 0; k < PER_THREAD; ++k) atomicAdd_system(&c[blockIdx.x], 1);
}
// ANCHOR_END: scope

// ANCHOR: cpu_gpu
// GPU 和 CPU 同时给 pinned 内存里的同一个计数器加 1。
__global__ void gpu_add_device(int* c, int n) {
    for (int k = 0; k < n; ++k) atomicAdd(c, 1);          // device 作用域
}
__global__ void gpu_add_system(int* c, int n) {
    for (int k = 0; k < n; ++k) atomicAdd_system(c, 1);   // system 作用域
}
// ANCHOR_END: cpu_gpu

int main() {
    const long long total = (long long)BLOCKS * THREADS * PER_THREAD;
    int* c;
    CHECK(cudaMalloc(&c, BLOCKS * THREADS * sizeof(int)));
    cudaEvent_t start, stop;
    CHECK(cudaEventCreate(&start));
    CHECK(cudaEventCreate(&stop));
    auto run = [&](const char* label, auto kernel, int* target) {
        float best = 1e30f;
        for (int r = 0; r < 3; ++r) {
            CHECK(cudaMemset(target, 0, BLOCKS * THREADS * sizeof(int)));
            CHECK(cudaEventRecord(start));
            kernel<<<BLOCKS, THREADS>>>(target);
            CHECK(cudaEventRecord(stop));
            CHECK(cudaEventSynchronize(stop));
            CHECK(cudaGetLastError());
            float ms;
            CHECK(cudaEventElapsedTime(&ms, start, stop));
            if (ms < best) best = ms;
        }
        printf("%8.2f ms  %7.1f 亿次/秒  %s\n", best, total / (best / 1e3) / 1e8, label);
    };
    printf("争用程度：\n");
    run("所有线程加同一个地址", one_address, c);
    run("每个 warp 一个地址", per_warp, c);
    run("每个线程一个地址", per_thread, c);
    run("warp 内先合并，再加同一个地址", warp_aggregated, c);
    printf("\n作用域（每个 block 一个计数器）：\n");
    run("atomicAdd_block（block 作用域）", scope_block, c);
    run("atomicAdd（device 作用域）", scope_device, c);
    run("atomicAdd_system（system 作用域）", scope_system, c);

    // CPU 和 GPU 同时加：各加 100 万次，结果应该是 200 万。
    int native;
    CHECK(cudaDeviceGetAttribute(&native, cudaDevAttrHostNativeAtomicSupported, 0));
    printf("\nGPU 对主机内存的原生原子操作：%s\n", native ? "支持" : "不支持");
    printf("CPU 和 GPU 同时给一个计数器各加 100 万次：\n");
    const int each = 1'000'000;
    for (bool managed : {false, true}) {
        int* counter;
        if (managed) CHECK(cudaMallocManaged(&counter, sizeof(int)));
        else CHECK(cudaMallocHost(&counter, sizeof(int)));
        for (bool system : {false, true}) {
            *counter = 0;
            if (system) gpu_add_system<<<1, 1>>>(counter, each);
            else gpu_add_device<<<1, 1>>>(counter, each);
            CHECK(cudaGetLastError());
            std::thread cpu([&] {
                cuda::atomic_ref<int, cuda::thread_scope_system> a(*counter);   // 主机端也能用
                for (int k = 0; k < each; ++k) a.fetch_add(1);
            });
            cpu.join();
            CHECK(cudaDeviceSynchronize());
            printf("  %-13s GPU 用 %-17s 结果 %d\n", managed ? "managed 内存" : "pinned 内存",
                   system ? "atomicAdd_system" : "atomicAdd", *counter);
        }
        if (managed) CHECK(cudaFree(counter));
        else CHECK(cudaFreeHost(counter));
    }
    CHECK(cudaFree(c));
}
