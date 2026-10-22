// gridsync.cu: 一维平滑迭代 1000 步。每一步都依赖上一步全部完成。
// 比较每步启动一个 kernel，和一个常驻的协作 kernel 在步与步之间用 grid.sync() 同步。
//   nvcc -O2 -arch=native gridsync.cu -o gridsync && ./gridsync [元素个数]
#include <cooperative_groups.h>
#include <cstdio>
#include <cstdlib>
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

constexpr int STEPS = 1000;

// 一步平滑：每个元素换成自己和左右邻居的平均值（两端不动）。
__device__ void smooth(const float* in, float* out, int n, int first, int stride) {
    for (int i = first; i < n; i += stride)
        out[i] = (i == 0 || i == n - 1) ? in[i] : (in[i - 1] + in[i] + in[i + 1]) / 3.0f;
}

// 做法一：每步一个 kernel，kernel 结束就是天然的全局同步。
__global__ void step_kernel(const float* in, float* out, int n) {
    smooth(in, out, n, blockIdx.x * blockDim.x + threadIdx.x, gridDim.x * blockDim.x);
}

// ANCHOR: persistent
// 做法二：一个 kernel 做完所有步，步与步之间等整个 grid 都做完这一步。
__global__ void all_steps(float* a, float* b, int n, int steps) {
    cg::grid_group grid = cg::this_grid();
    for (int s = 0; s < steps; ++s) {
        smooth(a, b, n, grid.thread_rank(), grid.num_threads());
        grid.sync();                         // 所有 block 的所有线程都到这里，才继续下一步
        float* t = a; a = b; b = t;
    }
}
// ANCHOR_END: persistent

int main(int argc, char** argv) {
    const int n = argc > 1 ? atoi(argv[1]) : 1 << 20;
    std::vector<float> h(n, 0.0f);
    h[n / 2] = 1000.0f;                      // 中间一个尖峰，看它被慢慢抹平
    float *a, *b;
    CHECK(cudaMalloc(&a, n * sizeof(float)));
    CHECK(cudaMalloc(&b, n * sizeof(float)));
    cudaEvent_t start, stop;
    CHECK(cudaEventCreate(&start));
    CHECK(cudaEventCreate(&stop));
    std::vector<float> r1(n), r2(n);

    // ANCHOR: size
    // 协作 kernel 的所有 block 必须同时驻留在 GPU 上，grid 大小不能超过这个上限。
    int per_sm, sms;
    CHECK(cudaOccupancyMaxActiveBlocksPerMultiprocessor(&per_sm, all_steps, 256, 0));
    CHECK(cudaDeviceGetAttribute(&sms, cudaDevAttrMultiProcessorCount, 0));
    int grid = per_sm * sms;
    // ANCHOR_END: size

    // 做法一
    CHECK(cudaMemcpy(a, h.data(), n * sizeof(float), cudaMemcpyHostToDevice));
    CHECK(cudaEventRecord(start));
    for (int s = 0; s < STEPS; ++s) {
        step_kernel<<<grid, 256>>>(a, b, n);
        float* t = a; a = b; b = t;
    }
    CHECK(cudaEventRecord(stop));
    CHECK(cudaEventSynchronize(stop));
    CHECK(cudaGetLastError());
    float ms1;
    CHECK(cudaEventElapsedTime(&ms1, start, stop));
    CHECK(cudaMemcpy(r1.data(), a, n * sizeof(float), cudaMemcpyDeviceToHost));

    // 做法二
    CHECK(cudaMemcpy(a, h.data(), n * sizeof(float), cudaMemcpyHostToDevice));
    int nn = n, steps = STEPS;
    // ANCHOR: launch
    void* args[] = {&a, &b, &nn, &steps};
    CHECK(cudaEventRecord(start));
    CHECK(cudaLaunchCooperativeKernel((void*)all_steps, grid, 256, args, 0, 0));
    CHECK(cudaEventRecord(stop));
    // ANCHOR_END: launch
    CHECK(cudaEventSynchronize(stop));
    float ms2;
    CHECK(cudaEventElapsedTime(&ms2, start, stop));
    float* result = STEPS % 2 == 0 ? a : b;   // 偶数步之后结果回到 a
    CHECK(cudaMemcpy(r2.data(), result, n * sizeof(float), cudaMemcpyDeviceToHost));

    // grid 超过能同时驻留的 block 数，协作启动会直接失败。
    cudaError_t too_big = cudaLaunchCooperativeKernel((void*)all_steps, grid + 1, 256, args, 0, 0);
    cudaGetLastError();

    int diff = 0;
    for (int i = 0; i < n; ++i) diff += r1[i] != r2[i];
    printf("%d 个元素，grid = %d 个 block（每个 SM %d 个）\n", n, grid, per_sm);
    printf("每步一个 kernel      %6.2f ms，每步 %5.2f us\n", ms1, ms1 * 1e3 / STEPS);
    printf("常驻 kernel + 网格同步 %6.2f ms，每步 %5.2f us\n", ms2, ms2 * 1e3 / STEPS);
    printf("两种做法的结果有 %d 个元素不同；中心值 %.4f\n", diff, r2[n / 2]);
    printf("用 %d 个 block 协作启动：%s\n", grid + 1, cudaGetErrorName(too_big));
    CHECK(cudaFree(a));
    CHECK(cudaFree(b));
}
