// sum_order.cu: 同样 400 多万个 float 求和，换一种加法顺序，结果就变了；用原子操作汇总时，每次运行的结果都可能不同。
//   nvcc -O2 -arch=native sum_order.cu -o sum_order && ./sum_order
#include <cub/cub.cuh>
#include <cstdio>
#include <cstdlib>
#include <random>
#include <set>
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

const int N = 1 << 22, BLOCKS = 1024, THREADS = 256;

template <class T>
__device__ T block_sum(T s) {                         // block 内求和，顺序固定
    __shared__ T part[THREADS];
    part[threadIdx.x] = s;
    __syncthreads();
    for (int d = THREADS / 2; d > 0; d /= 2) {
        if (threadIdx.x < d) part[threadIdx.x] += part[threadIdx.x + d];
        __syncthreads();
    }
    return part[0];
}

// ANCHOR: kernels
// 每个 block 先求出自己那部分的和，再用 atomicAdd 加到总和上：各 block 加进去的先后顺序由调度决定。
__global__ void sum_atomic(const float* x, float* total) {
    float s = 0;
    for (int i = blockIdx.x * THREADS + threadIdx.x; i < N; i += BLOCKS * THREADS) s += x[i];
    s = block_sum(s);
    if (threadIdx.x == 0) atomicAdd(total, s);
}
// 每个 block 把部分和写到自己的位置，第二个 kernel 按固定顺序把它们加起来：每次运行结果都一样。
__global__ void sum_partial(const float* x, float* partial) {
    float s = 0;
    for (int i = blockIdx.x * THREADS + threadIdx.x; i < N; i += BLOCKS * THREADS) s += x[i];
    s = block_sum(s);
    if (threadIdx.x == 0) partial[blockIdx.x] = s;
}
__global__ void sum_final(const float* partial, float* total) {
    float s = 0;
    for (int i = threadIdx.x; i < BLOCKS; i += THREADS) s += partial[i];
    s = block_sum(s);
    if (threadIdx.x == 0) *total = s;
}
// ANCHOR_END: kernels

int main() {
    std::vector<float> h(N);
    std::mt19937 rng(7);
    std::uniform_real_distribution<float> u(0, 1);
    for (float& v : h) v = u(rng);
    double ref = 0;                                   // 参考值：double 顺序累加，误差远小于 float 的舍入
    for (float v : h) ref += v;

    // ANCHOR: cpu
    float fwd = 0, bwd = 0, kahan = 0, comp = 0;
    for (int i = 0; i < N; ++i) fwd += h[i];
    for (int i = N - 1; i >= 0; --i) bwd += h[i];
    for (int i = 0; i < N; ++i) {                     // Kahan 求和：把每次加法丢掉的低位记下来，下次补回去
        float y = h[i] - comp, t = kahan + y;
        comp = (t - kahan) - y;
        kahan = t;
    }
    // ANCHOR_END: cpu
    printf("%d 个 [0,1) 之间的 float，double 顺序累加的参考值 %.6f\n", N, ref);
    auto show = [&](const char* name, float v) {
        printf("  %.6f  相对误差 %8.1e  %s", v, (v - ref) / ref, name);
    };
    show("CPU 从前往后\n", fwd);
    show("CPU 从后往前\n", bwd);
    show("CPU Kahan 求和\n", kahan);

    float *x, *partial, *total;
    CHECK(cudaMalloc(&x, N * sizeof(float)));
    CHECK(cudaMalloc(&partial, BLOCKS * sizeof(float)));
    CHECK(cudaMalloc(&total, sizeof(float)));
    CHECK(cudaMemcpy(x, h.data(), N * sizeof(float), cudaMemcpyHostToDevice));
    void* tmp = nullptr;
    size_t tmp_bytes = 0;
    CHECK(cub::DeviceReduce::Sum(tmp, tmp_bytes, x, total, N));
    CHECK(cudaMalloc(&tmp, tmp_bytes));
    // ANCHOR: runs
    const int runs = 20;
    const char* method[] = {"GPU 部分和 + atomicAdd", "GPU 部分和 + 固定顺序", "GPU CUB DeviceReduce"};
    for (int m = 0; m < 3; ++m) {
        std::set<float> seen;                         // 20 次运行得到了几种不同的结果
        float v = 0;
        for (int r = 0; r < runs; ++r) {
            if (m == 0) {
                CHECK(cudaMemset(total, 0, sizeof(float)));
                sum_atomic<<<BLOCKS, THREADS>>>(x, total);
            } else if (m == 1) {
                sum_partial<<<BLOCKS, THREADS>>>(x, partial);
                sum_final<<<1, THREADS>>>(partial, total);
            } else {
                CHECK(cub::DeviceReduce::Sum(tmp, tmp_bytes, x, total, N));
            }
            CHECK(cudaMemcpy(&v, total, sizeof v, cudaMemcpyDeviceToHost));
            seen.insert(v);
        }
        show(method[m], v);
        printf("（运行 %d 次，得到 %zu 种结果", runs, seen.size());
        if (seen.size() > 1) printf("，从 %.6f 到 %.6f", *seen.begin(), *seen.rbegin());
        printf("）\n");
    }
    // ANCHOR_END: runs
}
