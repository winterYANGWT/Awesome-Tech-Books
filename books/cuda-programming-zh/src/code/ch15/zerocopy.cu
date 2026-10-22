// zerocopy.cu: kernel 直接读 pinned 主机内存（零拷贝），和先拷进显存再算比较。
// 三种访问方式：顺序读一遍、反复读 10 遍、每 4 KB 只读一个 float。
//   nvcc -O2 -arch=native zerocopy.cu -o zerocopy && ./zerocopy
#include <chrono>
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

constexpr size_t N = size_t(64) << 20;    // 64M 个 float，256 MB

// ANCHOR: kernel
// 把 in 中每隔 step 个元素的那些加起来，重复 passes 遍。in 可以是显存，也可以是主机内存。
__global__ void sum_strided(const float* in, size_t step, int passes, float* out) {
    float s = 0;
    for (int p = 0; p < passes; ++p)
        for (size_t i = (blockIdx.x * size_t(blockDim.x) + threadIdx.x) * step; i < N;
             i += size_t(gridDim.x) * blockDim.x * step)
            s += in[i];
    atomicAdd(out, s);
}
// ANCHOR_END: kernel

double now_ms() {
    using namespace std::chrono;
    return duration<double, std::milli>(steady_clock::now().time_since_epoch()).count();
}

int main() {
    // ANCHOR: alloc
    // 64 位系统上，cudaMallocHost 分配的 pinned 内存同时映射进了 GPU 的地址空间，
    // kernel 可以直接拿这个指针去读写，数据经 PCIe 按需传输。
    float* host;
    CHECK(cudaMallocHost(&host, N * sizeof(float)));
    // ANCHOR_END: alloc
    for (size_t i = 0; i < N; ++i) host[i] = 1.0f;
    float *dev, *out;
    CHECK(cudaMalloc(&dev, N * sizeof(float)));
    CHECK(cudaMalloc(&out, sizeof(float)));
    const int grid = 26 * 6, block = 256;

    auto run = [&](size_t step, int passes, bool zero_copy) {
        double best = 1e30;
        for (int r = 0; r < 3; ++r) {
            CHECK(cudaMemset(out, 0, sizeof(float)));
            CHECK(cudaDeviceSynchronize());
            double t0 = now_ms();
            if (zero_copy) {
                sum_strided<<<grid, block>>>(host, step, passes, out);
            } else {
                CHECK(cudaMemcpy(dev, host, N * sizeof(float), cudaMemcpyHostToDevice));
                sum_strided<<<grid, block>>>(dev, step, passes, out);
            }
            CHECK(cudaGetLastError());
            CHECK(cudaDeviceSynchronize());
            double t = now_ms() - t0;
            if (t < best) best = t;
        }
        return best;
    };
    printf("访问方式                  先拷进显存   零拷贝\n");
    printf("顺序读一遍                %7.1f ms  %7.1f ms\n", run(1, 1, false), run(1, 1, true));
    printf("顺序读 10 遍              %7.1f ms  %7.1f ms\n", run(1, 10, false), run(1, 10, true));
    printf("每 4 KB 读一个 float      %7.1f ms  %7.1f ms\n", run(1024, 1, false), run(1024, 1, true));
    CHECK(cudaFreeHost(host));
    CHECK(cudaFree(dev));
    CHECK(cudaFree(out));
}
