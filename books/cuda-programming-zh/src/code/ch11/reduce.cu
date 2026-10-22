// reduce.cu: 把 6400 万个 float 加起来的几个版本，从最直接的写法一步步优化到接近显存带宽。
//   nvcc -O2 -arch=native reduce.cu -o reduce && ./reduce
#include <cub/cub.cuh>
#include <cmath>
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

// ANCHOR: v0
// 版本 0：每个线程把自己的元素直接加到结果上。
__global__ void reduce_atomic(const float* in, int n, float* out) {
    int i = blockIdx.x * blockDim.x + threadIdx.x;
    if (i < n) atomicAdd(out, in[i]);
}
// ANCHOR_END: v0

// ANCHOR: v1
// 版本 1：每个 block 在 shared memory 里两两相加，最后由 0 号线程把 block 的和加到结果上。
// 第 s 轮，下标是 2s 倍数的线程把右边隔 s 的元素加过来。
__global__ void reduce_tree(const float* in, int n, float* out) {
    __shared__ float s[THREADS];
    int t = threadIdx.x, i = blockIdx.x * blockDim.x + t;
    s[t] = i < n ? in[i] : 0.0f;
    __syncthreads();
    for (int stride = 1; stride < blockDim.x; stride *= 2) {
        if (t % (2 * stride) == 0) s[t] += s[t + stride];
        __syncthreads();
    }
    if (t == 0) atomicAdd(out, s[0]);
}
// ANCHOR_END: v1

// ANCHOR: v2
// 版本 2：同样两两相加，但让前一半线程干活：第一轮 0..127 号线程把 128..255 加过来，依此类推。
__global__ void reduce_tree_seq(const float* in, int n, float* out) {
    __shared__ float s[THREADS];
    int t = threadIdx.x, i = blockIdx.x * blockDim.x + t;
    s[t] = i < n ? in[i] : 0.0f;
    __syncthreads();
    for (int stride = blockDim.x / 2; stride > 0; stride /= 2) {
        if (t < stride) s[t] += s[t + stride];
        __syncthreads();
    }
    if (t == 0) atomicAdd(out, s[0]);
}
// ANCHOR_END: v2

// ANCHOR: v3
// 版本 3：grid 只开满一轮，每个线程先用 grid-stride 循环在寄存器里累加很多个元素，
// 然后再做一次 block 内的归约。
__global__ void reduce_stride(const float* in, int n, float* out) {
    __shared__ float s[THREADS];
    int t = threadIdx.x;
    float sum = 0.0f;
    for (int i = blockIdx.x * blockDim.x + t; i < n; i += gridDim.x * blockDim.x) sum += in[i];
    s[t] = sum;
    __syncthreads();
    for (int stride = blockDim.x / 2; stride > 0; stride /= 2) {
        if (t < stride) s[t] += s[t + stride];
        __syncthreads();
    }
    if (t == 0) atomicAdd(out, s[0]);
}
// ANCHOR_END: v3

// ANCHOR: v4
// warp 内的 32 个线程直接交换寄存器里的值：每轮把隔 offset 个 lane 的值加过来，5 轮后 lane 0 得到总和。
__device__ float warp_sum(float v) {
    for (int offset = 16; offset > 0; offset /= 2) v += __shfl_down_sync(0xffffffff, v, offset);
    return v;
}

// 版本 4：先在 warp 内用 shuffle 归约，每个 warp 的和放进 shared memory，再由第一个 warp 归约这 8 个数。
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
// ANCHOR_END: v4

// ANCHOR: v5
// 版本 5：在版本 4 的基础上，每次读一个 float4（16 字节），每轮有 4 个互不相干的加法。
__global__ void reduce_vec4(const float4* in, int n4, float* out) {
    __shared__ float warp_sums[THREADS / 32];
    int t = threadIdx.x, lane = t % 32, warp = t / 32;
    float sum = 0.0f;
    for (int i = blockIdx.x * blockDim.x + t; i < n4; i += gridDim.x * blockDim.x) {
        float4 v = in[i];
        sum += (v.x + v.y) + (v.z + v.w);
    }
    sum = warp_sum(sum);
    if (lane == 0) warp_sums[warp] = sum;
    __syncthreads();
    if (warp == 0) {
        sum = lane < THREADS / 32 ? warp_sums[lane] : 0.0f;
        sum = warp_sum(sum);
        if (lane == 0) atomicAdd(out, sum);
    }
}
// ANCHOR_END: v5

int main() {
    const int n = 64 << 20;                   // 6400 多万个 float，256 MB
    std::vector<float> h(n);
    for (int i = 0; i < n; ++i) h[i] = float((i * 7919LL) % 1000) / 1000.0f;
    double ref = 0;                           // 用 double 在 CPU 上算一个参考值
    for (float v : h) ref += v;

    float *in, *out;
    CHECK(cudaMalloc(&in, n * sizeof(float)));
    CHECK(cudaMalloc(&out, sizeof(float)));
    CHECK(cudaMemcpy(in, h.data(), n * sizeof(float), cudaMemcpyHostToDevice));
    int sms;
    CHECK(cudaDeviceGetAttribute(&sms, cudaDevAttrMultiProcessorCount, 0));
    const int one_wave = sms * 6;             // 256 线程的 block，每个 SM 驻留 6 个
    const int all = (n + THREADS - 1) / THREADS;

    cudaEvent_t start, stop;
    CHECK(cudaEventCreate(&start));
    CHECK(cudaEventCreate(&stop));
    auto run = [&](const char* label, auto launch) {
        float best = 1e30f, result = 0;
        for (int r = 0; r < 4; ++r) {         // 第一次兼作预热
            CHECK(cudaMemset(out, 0, sizeof(float)));
            CHECK(cudaEventRecord(start));
            launch();
            CHECK(cudaEventRecord(stop));
            CHECK(cudaEventSynchronize(stop));
            CHECK(cudaGetLastError());
            float ms;
            CHECK(cudaEventElapsedTime(&ms, start, stop));
            if (r > 0 && ms < best) best = ms;
            CHECK(cudaMemcpy(&result, out, sizeof(float), cudaMemcpyDeviceToHost));
        }
        printf("%-22s %8.3f ms  %5.0f GB/s  相对误差 %.1e\n", label, best,
               n * sizeof(float) / (best / 1e3) / 1e9, fabs(result - ref) / ref);
    };

    run("0 atomicAdd", [&] { reduce_atomic<<<all, THREADS>>>(in, n, out); });
    run("1 树形，取模", [&] { reduce_tree<<<all, THREADS>>>(in, n, out); });
    run("2 树形，前一半干活", [&] { reduce_tree_seq<<<all, THREADS>>>(in, n, out); });
    run("3 先在寄存器里累加", [&] { reduce_stride<<<one_wave, THREADS>>>(in, n, out); });
    run("4 warp shuffle", [&] { reduce_shuffle<<<one_wave, THREADS>>>(in, n, out); });
    run("5 float4", [&] {
        reduce_vec4<<<one_wave, THREADS>>>(reinterpret_cast<const float4*>(in), n / 4, out);
    });

    // ANCHOR: cub
    // 参照：CUB 库的归约。第一次调用只问需要多大的临时空间。
    void* temp = nullptr;
    size_t temp_bytes = 0;
    CHECK(cub::DeviceReduce::Sum(temp, temp_bytes, in, out, n));
    CHECK(cudaMalloc(&temp, temp_bytes));
    run("CUB DeviceReduce::Sum", [&] { CHECK(cub::DeviceReduce::Sum(temp, temp_bytes, in, out, n)); });
    // ANCHOR_END: cub
    CHECK(cudaFree(temp));
    CHECK(cudaFree(in));
    CHECK(cudaFree(out));
}
