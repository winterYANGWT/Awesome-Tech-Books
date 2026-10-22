// shapes.cu: 同一个二维 kernel，换不同的 block 形状和大小，看速度怎么变。
//   nvcc -O2 -arch=native shapes.cu -o shapes && ./shapes
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

// ANCHOR: kernel
// 对一幅 width×height 的图像逐像素做亮度变换，图像按行存放。
__global__ void brighten(const float* in, float* out, int width, int height) {
    int x = blockIdx.x * blockDim.x + threadIdx.x;
    int y = blockIdx.y * blockDim.y + threadIdx.y;
    if (x < width && y < height) {
        int i = y * width + x;
        out[i] = in[i] * 1.2f + 0.05f;
    }
}
// ANCHOR_END: kernel

int main() {
    const int width = 8192, height = 8192;            // 6400 多万个像素，256 MiB
    size_t bytes = size_t(width) * height * sizeof(float);
    float *in, *out;
    CHECK(cudaMalloc(&in, bytes));
    CHECK(cudaMalloc(&out, bytes));
    CHECK(cudaMemset(in, 0, bytes));
    cudaEvent_t start, stop;
    CHECK(cudaEventCreate(&start));
    CHECK(cudaEventCreate(&stop));

    auto run = [&](int bx, int by) {
        dim3 block(bx, by);
        dim3 grid((width + bx - 1) / bx, (height + by - 1) / by);
        brighten<<<grid, block>>>(in, out, width, height);   // 预热
        CHECK(cudaGetLastError());
        float best = 1e30f;
        for (int r = 0; r < 5; ++r) {
            CHECK(cudaEventRecord(start));
            brighten<<<grid, block>>>(in, out, width, height);
            CHECK(cudaEventRecord(stop));
            CHECK(cudaEventSynchronize(stop));
            float ms;
            CHECK(cudaEventElapsedTime(&ms, start, stop));
            if (ms < best) best = ms;
        }
        printf("%5d × %-4d  %6.2f ms  %5.0f GB/s\n", bx, by, best, 2.0 * bytes / (best / 1e3) / 1e9);
    };

    printf("每个 block 256 个线程，形状不同：\n");
    printf("block 形状     时间      带宽\n");
    for (int bx : {256, 128, 64, 32, 16, 8, 4, 2, 1}) run(bx, 256 / bx);

    printf("\n形状都是“一行”，大小不同：\n");
    for (int bx : {32, 64, 128, 256, 512, 1024}) run(bx, 1);

    CHECK(cudaFree(in));
    CHECK(cudaFree(out));
}
