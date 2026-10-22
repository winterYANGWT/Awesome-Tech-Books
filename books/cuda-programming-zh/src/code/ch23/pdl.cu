// pdl.cu: 一串前后依赖的小 kernel，每个开头有一段不依赖上一个 kernel 结果的准备工作。
// 比较普通启动和 Programmatic Dependent Launch（PDL）。
//   nvcc -O2 -arch=native pdl.cu -o pdl && ./pdl
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

constexpr int N = 26 * 256 * 4;
constexpr int KERNELS = 2000;

// ANCHOR: kernel
// trigger_early：一开头就允许下一个 kernel 被调度；否则在末尾才允许。
__global__ void step(float* x, float a, int prologue_iters, bool trigger_early) {
    if (trigger_early) cudaTriggerProgrammaticLaunchCompletion();

    // 准备工作：只依赖参数，不读上一个 kernel 的结果（比如算一些系数、读一张不变的表）。
    float c = a;
    for (int k = 0; k < prologue_iters; ++k) c = c * 0.999f + 0.001f;

    cudaGridDependencySynchronize();       // 从这里开始要用上一个 kernel 写的数据，必须等它完成

    int i = blockIdx.x * blockDim.x + threadIdx.x;
    if (i < N) x[i] = x[i] * 0.5f + c;
    if (!trigger_early) cudaTriggerProgrammaticLaunchCompletion();   // 告诉下一个 kernel：可以开始了
}
// ANCHOR_END: kernel

double now_ms() {
    using namespace std::chrono;
    return duration<double, std::milli>(steady_clock::now().time_since_epoch()).count();
}

int main() {
    float* x;
    CHECK(cudaMalloc(&x, N * sizeof(float)));
    cudaStream_t s;
    CHECK(cudaStreamCreateWithFlags(&s, cudaStreamNonBlocking));
    std::vector<float> r1(N), r2(N);

    const char* names[] = {"普通启动", "PDL，末尾发信号", "PDL，开头发信号"};
    printf("准备工作的迭代次数   %s   %s   %s（每个 kernel 的平均时间）\n", names[0], names[1], names[2]);
    for (int prologue : {0, 500, 2000}) {
        double t[3];
        for (int mode = 0; mode < 3; ++mode) {
            bool pdl = mode > 0, early = mode == 2;
            // ANCHOR: launch
            cudaLaunchConfig_t cfg = {};
            cfg.gridDim = N / 256;
            cfg.blockDim = 256;
            cfg.stream = s;
            cudaLaunchAttribute attr = {};
            attr.id = cudaLaunchAttributeProgrammaticStreamSerialization;
            attr.val.programmaticStreamSerializationAllowed = pdl;   // 允许和前一个 kernel 部分重叠
            cfg.attrs = &attr;
            cfg.numAttrs = 1;
            // ANCHOR_END: launch
            CHECK(cudaMemsetAsync(x, 0, N * sizeof(float), s));
            CHECK(cudaLaunchKernelEx(&cfg, step, x, 1.0f, prologue, early));   // 预热
            CHECK(cudaStreamSynchronize(s));
            CHECK(cudaMemsetAsync(x, 0, N * sizeof(float), s));
            double t0 = now_ms();
            for (int k = 0; k < KERNELS; ++k) CHECK(cudaLaunchKernelEx(&cfg, step, x, 1.0f, prologue, early));
            CHECK(cudaStreamSynchronize(s));
            t[mode] = now_ms() - t0;
            CHECK(cudaMemcpy(mode ? r2.data() : r1.data(), x, N * sizeof(float), cudaMemcpyDeviceToHost));
            int diff = 0;
            for (int i = 0; i < N; ++i) diff += r1[i] != r2[i];
            if (mode && diff) printf("  %s 的结果和普通启动有 %d 个不同\n", names[mode], diff);
        }
        printf("%10d          %7.2f us        %7.2f us          %7.2f us\n", prologue,
               t[0] * 1e3 / KERNELS, t[1] * 1e3 / KERNELS, t[2] * 1e3 / KERNELS);
    }
    CHECK(cudaFree(x));
}
