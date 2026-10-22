// tma_blur.cu: 3×3 均值模糊。每个 block 负责 32×32 个输出像素，需要连同一圈边框的 34×34 个输入像素。
// 比较三种取数据的方式：直接读 global、线程合作手工加载、用 TMA 加载。
//   nvcc -O2 -arch=native tma_blur.cu -o tma_blur -lcuda && ./tma_blur
// NVCCFLAGS: -lcuda
#include <cuda.h>
#include <cuda/barrier>
#include <cuda/ptx>
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

using block_barrier = cuda::barrier<cuda::thread_scope_block>;

constexpr int T = 32;              // 每个 block 的输出块是 T×T
constexpr int BOX_W = 36;          // 手工加载的输入块宽度：T+2 再补成 4 的倍数
constexpr int BOX_H = T + 2;
constexpr int TMA_W = 40;          // TMA 的输入块：从 x-4 开始（最内层坐标要 16 字节对齐），到 x+35 为止

// 从输入块 tile（左上角对应输出块左上角往左上各偏一格）算一个输出像素。
__device__ float blur_at(const float* tile, int pitch, int x, int y) {
    float s = 0;
    for (int dy = 0; dy < 3; ++dy)
        for (int dx = 0; dx < 3; ++dx) s += tile[(y + dy) * pitch + x + dx];
    return s / 9.0f;
}

// 做法一：每个线程直接从 global memory 读 9 个邻居，越界的当作 0。
__global__ void blur_global(const float* in, float* out, int w, int h) {
    int x0 = blockIdx.x * T, y0 = blockIdx.y * T;
    for (int ty = threadIdx.y; ty < T; ty += blockDim.y) {
        int x = x0 + threadIdx.x, y = y0 + ty;
        float s = 0;
        for (int dy = -1; dy <= 1; ++dy)
            for (int dx = -1; dx <= 1; ++dx) {
                int xx = x + dx, yy = y + dy;
                if (xx >= 0 && xx < w && yy >= 0 && yy < h) s += in[size_t(yy) * w + xx];
            }
        out[size_t(y) * w + x] = s / 9.0f;
    }
}

// ANCHOR: manual
// 做法二：block 里的线程合作，把 34×34 的输入块读进 shared memory，自己处理边界。
__global__ void blur_manual(const float* in, float* out, int w, int h) {
    __shared__ float tile[BOX_H * BOX_W];
    int x0 = blockIdx.x * T - 1, y0 = blockIdx.y * T - 1;   // 输入块左上角，可能是 -1
    int tid = threadIdx.y * blockDim.x + threadIdx.x, nthreads = blockDim.x * blockDim.y;
    for (int i = tid; i < BOX_H * BOX_W; i += nthreads) {
        int xx = x0 + i % BOX_W, yy = y0 + i / BOX_W;
        tile[i] = (xx >= 0 && xx < w && yy >= 0 && yy < h) ? in[size_t(yy) * w + xx] : 0.0f;
    }
    __syncthreads();
    for (int ty = threadIdx.y; ty < T; ty += blockDim.y)
        out[size_t(y0 + 1 + ty) * w + x0 + 1 + threadIdx.x] = blur_at(tile, BOX_W, threadIdx.x, ty);
}
// ANCHOR_END: manual

// ANCHOR: tma
// 做法三：一个线程发起一次 TMA 拷贝，硬件按描述符把 36×34 的块搬进 shared memory，越界部分自动填 0。
__global__ void blur_tma(const __grid_constant__ CUtensorMap in_map, float* out, int w) {
    __shared__ alignas(128) float tile[BOX_H * TMA_W];
    __shared__ alignas(block_barrier) unsigned char bar_storage[sizeof(block_barrier)];
    block_barrier& bar = *reinterpret_cast<block_barrier*>(bar_storage);
    bool leader = threadIdx.x == 0 && threadIdx.y == 0;
    if (leader) init(&bar, blockDim.x * blockDim.y);
    __syncthreads();

    int x0 = blockIdx.x * T - 4, y0 = blockIdx.y * T - 1;
    block_barrier::arrival_token token;
    if (leader) {
        int32_t coords[2] = {x0, y0};                      // 可以是负数，但 x 必须是 4 的倍数
        cuda::ptx::cp_async_bulk_tensor(cuda::ptx::space_cluster, cuda::ptx::space_global, tile, &in_map,
                                        coords, cuda::device::barrier_native_handle(bar));
        token = cuda::device::barrier_arrive_tx(bar, 1, sizeof(tile));   // 告诉 barrier 要等这么多字节到达
    } else {
        token = bar.arrive();
    }
    bar.wait(std::move(token));                            // 数据到齐、所有线程到达

    for (int ty = threadIdx.y; ty < T; ty += blockDim.y)          // 输出 x 对应块里的第 x+3 列
        out[size_t(y0 + 1 + ty) * w + x0 + 4 + threadIdx.x] = blur_at(tile, TMA_W, threadIdx.x + 3, ty);
}
// ANCHOR_END: tma

int main() {
    const int w = 8192, h = 8192;
    size_t bytes = size_t(w) * h * sizeof(float);
    std::vector<float> hin(size_t(w) * h), hout(hin.size());
    for (size_t i = 0; i < hin.size(); ++i) hin[i] = float(i % 251);
    float *in, *out;
    CHECK(cudaMalloc(&in, bytes));
    CHECK(cudaMalloc(&out, bytes));
    CHECK(cudaMemcpy(in, hin.data(), bytes, cudaMemcpyHostToDevice));

    // ANCHOR: encode
    // 在主机上描述这块二维数据：尺寸、行跨度、每次搬的块大小。
    CUtensorMap in_map;
    cuuint64_t dims[2] = {cuuint64_t(w), cuuint64_t(h)};          // 最内层（x）在前
    cuuint64_t strides[1] = {cuuint64_t(w) * sizeof(float)};      // 第二维每加 1 跨多少字节
    cuuint32_t box[2] = {TMA_W, BOX_H};
    cuuint32_t elem_strides[2] = {1, 1};
    CUresult r = cuTensorMapEncodeTiled(&in_map, CU_TENSOR_MAP_DATA_TYPE_FLOAT32, 2, in, dims, strides, box,
                                        elem_strides, CU_TENSOR_MAP_INTERLEAVE_NONE, CU_TENSOR_MAP_SWIZZLE_NONE,
                                        CU_TENSOR_MAP_L2_PROMOTION_L2_128B, CU_TENSOR_MAP_FLOAT_OOB_FILL_NONE);
    if (r != CUDA_SUCCESS) { fprintf(stderr, "cuTensorMapEncodeTiled 失败：%d\n", int(r)); return 1; }
    // ANCHOR_END: encode

    dim3 grid(w / T, h / T), block(T, 8);
    cudaEvent_t start, stop;
    CHECK(cudaEventCreate(&start));
    CHECK(cudaEventCreate(&stop));
    auto run = [&](const char* label, auto launch) {
        float best = 1e30f;
        for (int k = 0; k < 4; ++k) {
            CHECK(cudaEventRecord(start));
            launch();
            CHECK(cudaEventRecord(stop));
            CHECK(cudaEventSynchronize(stop));
            CHECK(cudaGetLastError());
            float ms;
            CHECK(cudaEventElapsedTime(&ms, start, stop));
            if (k > 0 && ms < best) best = ms;
        }
        CHECK(cudaMemcpy(hout.data(), out, bytes, cudaMemcpyDeviceToHost));
        size_t bad = 0;
        for (int y : {0, 1, 4095, h - 1})
            for (int x = 0; x < w; x += 7) {
                float s = 0;
                for (int dy = -1; dy <= 1; ++dy)
                    for (int dx = -1; dx <= 1; ++dx) {
                        int xx = x + dx, yy = y + dy;
                        if (xx >= 0 && xx < w && yy >= 0 && yy < h) s += hin[size_t(yy) * w + xx];
                    }
                bad += std::abs(s / 9.0f - hout[size_t(y) * w + x]) > 1e-3f;
            }
        printf("%6.3f ms  %5.0f GB/s  %s%s\n", best, 2.0 * bytes / (best / 1e3) / 1e9, label, bad ? "  结果错误" : "");
    };
    run("直接读 global", [&] { blur_global<<<grid, block>>>(in, out, w, h); });
    run("手工加载到 shared memory", [&] { blur_manual<<<grid, block>>>(in, out, w, h); });
    run("TMA 加载到 shared memory", [&] { blur_tma<<<grid, block>>>(in_map, out, w); });
    CHECK(cudaFree(in));
    CHECK(cudaFree(out));
}
