// cases.cu: 四个 kernel，各代表一类瓶颈，用来练习看 Nsight Compute 的报告。
//   nvcc -O2 -lineinfo -arch=native cases.cu -o cases
//   ncu --section SpeedOfLight --section WarpStateStats ./cases
// NVCCFLAGS: -lineinfo
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

// ANCHOR: kernels
// 1. 访存为主：grid-stride 拷贝，每个 SM 驻留满 48 个 warp。
__global__ void bandwidth_bound(const float* in, float* out, size_t n) {
    for (size_t i = blockIdx.x * size_t(blockDim.x) + threadIdx.x; i < n; i += size_t(gridDim.x) * blockDim.x)
        out[i] = in[i];
}

// 2. 同样的拷贝，但每个 SM 只驻留 8 个 warp（启动时用动态 shared memory 占位）。
__global__ void latency_bound(const float* in, float* out, size_t n) {
    extern __shared__ char unused[];
    for (size_t i = blockIdx.x * size_t(blockDim.x) + threadIdx.x; i < n; i += size_t(gridDim.x) * blockDim.x)
        out[i] = in[i];
}

// 3. 计算为主：四条独立的乘加链。
__global__ void compute_bound(float* out) {
    float a = threadIdx.x, b = a + 1, c = a + 2, d = a + 3;
    for (int k = 0; k < 4096; ++k) {
        a = a * 0.9999f + 0.5f;
        b = b * 0.9999f + 0.5f;
        c = c * 0.9999f + 0.5f;
        d = d * 0.9999f + 0.5f;
    }
    out[blockIdx.x * blockDim.x + threadIdx.x] = a + b + c + d;
}

// 4. 第 11 章版本 1 的树形归约：每 256 个元素做 8 轮相加、8 次同步。
__global__ void sync_bound(const float* in, int n, float* out) {
    __shared__ float s[256];
    int t = threadIdx.x, i = blockIdx.x * blockDim.x + t;
    s[t] = i < n ? in[i] : 0.0f;
    __syncthreads();
    for (int stride = 1; stride < blockDim.x; stride *= 2) {
        if (t % (2 * stride) == 0) s[t] += s[t + stride];
        __syncthreads();
    }
    if (t == 0) atomicAdd(out, s[0]);
}
// ANCHOR_END: kernels

int main() {
    const size_t n = size_t(64) << 20;
    float *in, *out;
    CHECK(cudaMalloc(&in, n * sizeof(float)));
    CHECK(cudaMalloc(&out, n * sizeof(float)));
    CHECK(cudaMemset(in, 0, n * sizeof(float)));
    int sms;
    CHECK(cudaDeviceGetAttribute(&sms, cudaDevAttrMultiProcessorCount, 0));
    size_t big = 99 * 1024;                    // 每个 block 占 99 KB，一个 SM 只放得下一个
    CHECK(cudaFuncSetAttribute(latency_bound, cudaFuncAttributeMaxDynamicSharedMemorySize, int(big)));

    bandwidth_bound<<<sms * 6, 256>>>(in, out, n);
    latency_bound<<<sms, 256, big>>>(in, out, n);
    compute_bound<<<sms * 6 * 8, 256>>>(out);
    sync_bound<<<int(n / 256), 256>>>(in, int(n), out);
    CHECK(cudaGetLastError());
    CHECK(cudaDeviceSynchronize());
    printf("done\n");
    CHECK(cudaFree(in));
    CHECK(cudaFree(out));
}
