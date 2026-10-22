// cdp.cu: 动态并行（kernel 里启动 kernel）。先比较主机和设备启动 kernel 的开销，
// 再处理长度极不均匀的一组段：每段求和，比较一个线程一段、一个 block 一段、长段由 kernel 再启动子 grid 三种做法。
//   nvcc -O2 -arch=native -rdc=true cdp.cu -lcudadevrt -o cdp && ./cdp
// NVCCFLAGS: -rdc=true -lcudadevrt
#include <chrono>
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

static double now_ms() {
    return std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now().time_since_epoch()).count();
}

__global__ void empty() {}

// ANCHOR: launch
__global__ void launch_from_one_thread(int n) {       // 一个线程依次启动 n 个子 kernel
    for (int i = 0; i < n; ++i) empty<<<1, 32, 0, cudaStreamFireAndForget>>>();
}
__global__ void launch_from_many_threads() {          // 每个线程启动一个子 kernel
    empty<<<1, 32, 0, cudaStreamFireAndForget>>>();
}
// ANCHOR_END: launch

// ANCHOR: segments
// 段 i 是 data[off[i] .. off[i+1])，结果写到 sum[i]。
__global__ void per_thread(const float* data, const int* off, int nseg, float* sum) {
    int i = blockIdx.x * blockDim.x + threadIdx.x;
    if (i >= nseg) return;
    float s = 0;
    for (int k = off[i]; k < off[i + 1]; ++k) s += data[k];
    sum[i] = s;
}

__device__ float block_sum(float s) {                 // block 内求和，结果在 0 号线程
    __shared__ float part[32];
    for (int d = 16; d > 0; d /= 2) s += __shfl_down_sync(0xffffffff, s, d);
    if (threadIdx.x % 32 == 0) part[threadIdx.x / 32] = s;
    __syncthreads();
    s = threadIdx.x < blockDim.x / 32 ? part[threadIdx.x] : 0;
    if (threadIdx.x < 32)
        for (int d = 16; d > 0; d /= 2) s += __shfl_down_sync(0xffffffff, s, d);
    return s;
}

__global__ void per_block(const float* data, const int* off, float* sum) {
    float s = 0;
    for (int k = off[blockIdx.x] + threadIdx.x; k < off[blockIdx.x + 1]; k += blockDim.x) s += data[k];
    s = block_sum(s);
    if (threadIdx.x == 0) sum[blockIdx.x] = s;
}

__global__ void child_sum(const float* data, int begin, int end, float* out) {   // 子 grid：多个 block 合作求一段的和
    float s = 0;
    for (int k = begin + blockIdx.x * blockDim.x + threadIdx.x; k < end; k += gridDim.x * blockDim.x) s += data[k];
    s = block_sum(s);
    if (threadIdx.x == 0) atomicAdd(out, s);
}

// 短段由一个 block 自己算；长段由 block 的 0 号线程启动一个子 grid，按长度决定子 grid 的大小。
__global__ void per_block_cdp(const float* data, const int* off, float* sum, int threshold) {
    int begin = off[blockIdx.x], end = off[blockIdx.x + 1];
    if (end - begin > threshold) {
        if (threadIdx.x == 0) {
            sum[blockIdx.x] = 0;
            int blocks = min((end - begin) / (256 * 16), 1024);
            child_sum<<<blocks, 256, 0, cudaStreamFireAndForget>>>(data, begin, end, &sum[blockIdx.x]);
        }
        return;
    }
    float s = 0;
    for (int k = begin + threadIdx.x; k < end; k += blockDim.x) s += data[k];
    s = block_sum(s);
    if (threadIdx.x == 0) sum[blockIdx.x] = s;
}
// ANCHOR_END: segments

template <class F>
double time_ms(F f, int reps = 10) {
    f();
    CHECK(cudaGetLastError());
    CHECK(cudaDeviceSynchronize());
    double t0 = now_ms();
    for (int r = 0; r < reps; ++r) f();
    CHECK(cudaDeviceSynchronize());
    return (now_ms() - t0) / reps;
}

int main() {
    const int n = 4096;
    double t_host = time_ms([&] { for (int i = 0; i < n; ++i) empty<<<1, 32>>>(); });
    double t_one = time_ms([&] { launch_from_one_thread<<<1, 1>>>(n); });
    double t_many = time_ms([&] { launch_from_many_threads<<<n / 128, 128>>>(); });
    printf("启动 %d 个空 kernel：主机上逐个启动 %.2f ms，一个 GPU 线程逐个启动 %.2f ms，%d 个 GPU 线程各启动一个 %.2f ms\n",
           n, t_host, t_one, n, t_many);

    // 4096 段：每 64 段里有一段长 2^19，其余长 256，共约 3300 万个数
    const int nseg = 4096;
    std::vector<int> off(nseg + 1, 0);
    for (int i = 0; i < nseg; ++i) off[i + 1] = off[i] + (i % 64 == 0 ? 1 << 19 : 256);
    int total = off[nseg];
    float *data, *sum;
    int* d_off;
    CHECK(cudaMalloc(&data, size_t(total) * sizeof(float)));
    CHECK(cudaMalloc(&sum, nseg * sizeof(float)));
    CHECK(cudaMalloc(&d_off, (nseg + 1) * sizeof(int)));
    CHECK(cudaMemcpy(d_off, off.data(), (nseg + 1) * sizeof(int), cudaMemcpyHostToDevice));
    std::vector<float> ones(total, 1.0f), h(nseg);
    CHECK(cudaMemcpy(data, ones.data(), size_t(total) * sizeof(float), cudaMemcpyHostToDevice));
    auto check = [&] {                                // 数据全是 1，每段的和就是段长
        CHECK(cudaMemcpy(h.data(), sum, nseg * sizeof(float), cudaMemcpyDeviceToHost));
        int bad = 0;
        for (int i = 0; i < nseg; ++i) bad += h[i] != float(off[i + 1] - off[i]);
        return bad;
    };
    printf("\n%d 段，共 %d 个 float（%.0f MB），最长的段 %d，最短的 %d：\n", nseg, total, total * 4.0 / 1e6, 1 << 19, 256);
    double ta = time_ms([&] { per_thread<<<nseg / 128, 128>>>(data, d_off, nseg, sum); }, 3);
    printf("  一个线程一段              %8.3f ms，错误 %d 段\n", ta, check());
    double tb = time_ms([&] { per_block<<<nseg, 256>>>(data, d_off, sum); });
    printf("  一个 block 一段           %8.3f ms，错误 %d 段\n", tb, check());
    double tc = time_ms([&] { per_block_cdp<<<nseg, 256>>>(data, d_off, sum, 65536); });
    printf("  长段再启动子 grid（CDP）   %8.3f ms，错误 %d 段\n", tc, check());
    printf("  （读一遍这些数据按 336 GB/s 算要 %.3f ms）\n", total * 4.0 / 336e9 * 1e3);
}
