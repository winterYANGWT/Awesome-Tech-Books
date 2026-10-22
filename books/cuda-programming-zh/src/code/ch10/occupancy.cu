// occupancy.cu: 用动态 shared memory 限制每个 SM 能驻留的 block 数，
// 看访存为主和计算为主的 kernel 在不同 occupancy 下的表现。
//   nvcc -O2 -arch=native occupancy.cu -o occupancy && ./occupancy
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

constexpr int THREADS = 256;
constexpr int ITERS = 4096;

// ANCHOR: kernels
// 访存为主：grid-stride 拷贝。动态 shared memory 只是用来占地方，kernel 并不使用它。
__global__ void copy(const float* in, float* out, size_t n) {
    extern __shared__ char unused[];
    size_t stride = size_t(gridDim.x) * blockDim.x;
    for (size_t i = blockIdx.x * size_t(blockDim.x) + threadIdx.x; i < n; i += stride)
        out[i] = in[i];
}

// 同样是拷贝，每个线程一轮先发出 4 个互不相干的读，再写 4 个。
__global__ void copy4(const float* in, float* out, size_t n) {
    extern __shared__ char unused[];
    size_t stride = size_t(gridDim.x) * blockDim.x;
    size_t i = blockIdx.x * size_t(blockDim.x) + threadIdx.x;
    for (; i + 3 * stride < n; i += 4 * stride) {
        float a = in[i], b = in[i + stride], c = in[i + 2 * stride], d = in[i + 3 * stride];
        out[i] = a, out[i + stride] = b, out[i + 2 * stride] = c, out[i + 3 * stride] = d;
    }
    for (; i < n; i += stride) out[i] = in[i];
}

// 计算为主，一条依赖链：每次乘加都要等上一次的结果。
__global__ void chain1(float* out) {
    extern __shared__ char unused[];
    float a = threadIdx.x;
    for (int k = 0; k < ITERS; ++k) a = a * 0.9999f + 0.5f;
    out[blockIdx.x * blockDim.x + threadIdx.x] = a;
}

// 计算为主，四条互不相干的链：总运算量是 chain1 的 4 倍，但相邻的乘加可以同时在流水线里。
__global__ void chain4(float* out) {
    extern __shared__ char unused[];
    float a = threadIdx.x, b = a + 1, c = a + 2, d = a + 3;
    for (int k = 0; k < ITERS; ++k) {
        a = a * 0.9999f + 0.5f;
        b = b * 0.9999f + 0.5f;
        c = c * 0.9999f + 0.5f;
        d = d * 0.9999f + 0.5f;
    }
    out[blockIdx.x * blockDim.x + threadIdx.x] = a + b + c + d;
}
// ANCHOR_END: kernels

int main() {
    cudaDeviceProp prop;
    CHECK(cudaGetDeviceProperties(&prop, 0));
    const int sms = prop.multiProcessorCount;
    const size_t n = size_t(64) << 20;
    float *in, *out;
    CHECK(cudaMalloc(&in, n * sizeof(float)));
    CHECK(cudaMalloc(&out, n * sizeof(float)));
    CHECK(cudaMemset(in, 0, n * sizeof(float)));
    for (auto f : {(const void*)copy, (const void*)copy4, (const void*)chain1, (const void*)chain4})
        CHECK(cudaFuncSetAttribute(f, cudaFuncAttributeMaxDynamicSharedMemorySize,
                                   int(prop.sharedMemPerBlockOptin)));
    cudaEvent_t start, stop;
    CHECK(cudaEventCreate(&start));
    CHECK(cudaEventCreate(&stop));
    auto time = [&](auto launch) {
        launch();
        CHECK(cudaGetLastError());
        float best = 1e30f;
        for (int r = 0; r < 5; ++r) {
            CHECK(cudaEventRecord(start));
            launch();
            CHECK(cudaEventRecord(stop));
            CHECK(cudaEventSynchronize(stop));
            float ms;
            CHECK(cudaEventElapsedTime(&ms, start, stop));
            if (ms < best) best = ms;
        }
        return best;
    };

    printf("驻留 warp   copy      copy4      chain1         chain4\n");
    for (int per_sm = 1; per_sm <= 6; ++per_sm) {
        // ANCHOR: limit
        // 找出能让每个 SM 恰好放下 per_sm 个 block 的最大动态 shared memory 用量。
        size_t smem = prop.sharedMemPerBlockOptin;
        int blocks_per_sm = 0;
        for (;; smem -= 256) {
            CHECK(cudaOccupancyMaxActiveBlocksPerMultiprocessor(&blocks_per_sm, copy, THREADS, smem));
            if (blocks_per_sm >= per_sm) break;
        }
        // ANCHOR_END: limit
        int warps = blocks_per_sm * THREADS / 32;
        int grid = sms * blocks_per_sm;               // 正好填满一轮
        float tc = time([&] { copy<<<grid, THREADS, smem>>>(in, out, n); });
        float tc4 = time([&] { copy4<<<grid, THREADS, smem>>>(in, out, n); });
        float t1 = time([&] { chain1<<<grid * 8, THREADS, smem>>>(out); });
        float t4 = time([&] { chain4<<<grid * 8, THREADS, smem>>>(out); });
        double gb = 2.0 * n * sizeof(float) / 1e9;
        double ops = double(grid) * 8 * THREADS * ITERS * 2;   // 每次乘加算 2 次浮点运算
        printf("%4d/48  %4.0f GB/s  %4.0f GB/s  %5.2f TFLOPS  %5.2f TFLOPS\n", warps,
               gb / (tc / 1e3), gb / (tc4 / 1e3), ops / (t1 / 1e3) / 1e12, 4 * ops / (t4 / 1e3) / 1e12);
    }
    CHECK(cudaFree(in));
    CHECK(cudaFree(out));
}
