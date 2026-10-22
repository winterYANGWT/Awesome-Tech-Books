// transpose.cu: 矩阵转置的几个版本：朴素写法、用 shared memory 分块、再消除 bank conflict。
//   nvcc -O2 -arch=native transpose.cu -o transpose && ./transpose [n]
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

constexpr int TILE = 32;     // 每个 block 负责 32×32 的一块
constexpr int ROWS = 8;      // block 是 32×8 个线程，每个线程处理 4 个元素

// ANCHOR: copy
// 参照：同样的访问方式，只是不转置。读和写都是合并的。
__global__ void copy(const float* in, float* out, int n) {
    int x = blockIdx.x * TILE + threadIdx.x;
    int y = blockIdx.y * TILE + threadIdx.y;
    for (int j = 0; j < TILE; j += ROWS)
        out[(y + j) * n + x] = in[(y + j) * n + x];
}
// ANCHOR_END: copy

// ANCHOR: naive
// 朴素转置：读 in 的一行是合并的，写到 out 的一列是跨步的。
__global__ void transpose_naive(const float* in, float* out, int n) {
    int x = blockIdx.x * TILE + threadIdx.x;
    int y = blockIdx.y * TILE + threadIdx.y;
    for (int j = 0; j < TILE; j += ROWS)
        out[x * n + (y + j)] = in[(y + j) * n + x];
}
// ANCHOR_END: naive

// ANCHOR: tiled
// 分块转置：先把 32×32 的一块按行读进 shared memory，再按列从 shared memory 取出来，
// 按行写到 out 的对应位置。global memory 的读和写都是合并的。
template <int PAD>
__global__ void transpose_tiled(const float* in, float* out, int n) {
    __shared__ float tile[TILE][TILE + PAD];
    int x = blockIdx.x * TILE + threadIdx.x;
    int y = blockIdx.y * TILE + threadIdx.y;
    for (int j = 0; j < TILE; j += ROWS)
        tile[threadIdx.y + j][threadIdx.x] = in[(y + j) * n + x];
    __syncthreads();                       // 整块都读进来以后，才能开始按列取
    x = blockIdx.y * TILE + threadIdx.x;   // 输出块的位置：行列对调
    y = blockIdx.x * TILE + threadIdx.y;
    for (int j = 0; j < TILE; j += ROWS)
        out[(y + j) * n + x] = tile[threadIdx.x][threadIdx.y + j];
}
// ANCHOR_END: tiled

int main(int argc, char** argv) {
    const int n = argc > 1 ? atoi(argv[1]) : 8192;   // 默认 8192×8192 的 float 矩阵，256 MB
    printf("%d×%d\n", n, n);
    size_t bytes = size_t(n) * n * sizeof(float);
    std::vector<float> h(size_t(n) * n);
    for (size_t i = 0; i < h.size(); ++i) h[i] = float(i % 65521);
    float *in, *out;
    CHECK(cudaMalloc(&in, bytes));
    CHECK(cudaMalloc(&out, bytes));
    CHECK(cudaMemcpy(in, h.data(), bytes, cudaMemcpyHostToDevice));
    cudaEvent_t start, stop;
    CHECK(cudaEventCreate(&start));
    CHECK(cudaEventCreate(&stop));

    dim3 grid(n / TILE, n / TILE), block(TILE, ROWS);
    std::vector<float> res(h.size());
    auto run = [&](const char* label, auto kernel, bool transposed) {
        CHECK(cudaMemset(out, 0, bytes));
        kernel<<<grid, block>>>(in, out, n);
        CHECK(cudaGetLastError());
        const int reps = 20;                  // 小矩阵一次只要几十微秒，连续跑 20 次取平均
        CHECK(cudaEventRecord(start));
        for (int r = 0; r < reps; ++r) kernel<<<grid, block>>>(in, out, n);
        CHECK(cudaEventRecord(stop));
        CHECK(cudaEventSynchronize(stop));
        float ms;
        CHECK(cudaEventElapsedTime(&ms, start, stop));
        float best = ms / reps;
        CHECK(cudaMemcpy(res.data(), out, bytes, cudaMemcpyDeviceToHost));
        size_t bad = 0;
        for (int r = 0; r < n; r += 7)
            for (int c = 0; c < n; c += 5)
                bad += res[size_t(r) * n + c] != (transposed ? h[size_t(c) * n + r] : h[size_t(r) * n + c]);
        printf("%-20s %7.3f ms  %5.0f GB/s%s\n", label, best, 2.0 * bytes / (best / 1e3) / 1e9,
               bad ? "  结果错误" : "");
    };
    run("copy (参照)", copy, false);
    run("transpose_naive", transpose_naive, true);
    run("transpose_tiled<0>", transpose_tiled<0>, true);
    run("transpose_tiled<1>", transpose_tiled<1>, true);
    CHECK(cudaFree(in));
    CHECK(cudaFree(out));
}
