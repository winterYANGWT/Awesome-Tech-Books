// mandelbrot.cu: 用二维 grid 画 Mandelbrot 集合，每个线程算一个像素，结果写成 PPM 图片。
//   nvcc -O2 -arch=native mandelbrot.cu -o mandelbrot && ./mandelbrot 1920 1080
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

constexpr int MAX_ITERS = 512;

// 迭代次数映射成颜色：越快逃逸越暗，不逃逸（属于集合）的点是黑色。
__host__ __device__ uchar3 color(int iters) {
    if (iters == MAX_ITERS) return make_uchar3(0, 0, 0);
    float t = float(iters) / 64.0f;
    t = t - int(t);                       // 每 64 次迭代循环一遍色带
    return make_uchar3(255 * t * t, 255 * t, 128 + 127 * t);
}

__host__ __device__ int escape(float cx, float cy) {
    float x = 0, y = 0;
    int k = 0;
    while (k < MAX_ITERS && x * x + y * y < 4.0f) {
        float xt = x * x - y * y + cx;
        y = 2 * x * y + cy;
        x = xt;
        ++k;
    }
    return k;
}

// ANCHOR: kernel
// 像素 (px, py) 对应复平面上的点 (x0 + px*step, y0 + py*step)。
__global__ void mandelbrot(uchar3* img, int width, int height,
                           float x0, float y0, float step) {
    int px = blockIdx.x * blockDim.x + threadIdx.x;   // 列
    int py = blockIdx.y * blockDim.y + threadIdx.y;   // 行
    if (px >= width || py >= height) return;
    int k = escape(x0 + px * step, y0 + py * step);
    img[py * width + px] = color(k);                  // 按行存放
}
// ANCHOR_END: kernel

int main(int argc, char** argv) {
    int width = argc > 1 ? atoi(argv[1]) : 1920;
    int height = argc > 2 ? atoi(argv[2]) : 1080;
    float step = 3.0f / width;                        // 横向覆盖 [-2.2, 0.8]
    float x0 = -2.2f, y0 = -step * height / 2;

    uchar3* d_img;
    CHECK(cudaMalloc(&d_img, size_t(width) * height * sizeof(uchar3)));
    CHECK(cudaFree(0));

    // ANCHOR: launch
    dim3 block(16, 16);                               // 每个 block 16×16 = 256 个线程
    dim3 grid((width + block.x - 1) / block.x,        // 横向需要几个 block
              (height + block.y - 1) / block.y);      // 纵向需要几个 block
    mandelbrot<<<grid, block>>>(d_img, width, height, x0, y0, step);
    CHECK(cudaGetLastError());
    // ANCHOR_END: launch
    CHECK(cudaDeviceSynchronize());

    cudaEvent_t start, stop;
    CHECK(cudaEventCreate(&start));
    CHECK(cudaEventCreate(&stop));
    CHECK(cudaEventRecord(start));
    mandelbrot<<<grid, block>>>(d_img, width, height, x0, y0, step);
    CHECK(cudaEventRecord(stop));
    CHECK(cudaEventSynchronize(stop));
    float gpu_ms;
    CHECK(cudaEventElapsedTime(&gpu_ms, start, stop));

    std::vector<uchar3> img(size_t(width) * height), ref(img.size());
    CHECK(cudaMemcpy(img.data(), d_img, img.size() * sizeof(uchar3), cudaMemcpyDeviceToHost));

    // 同样的计算在 CPU 单线程上跑一遍，顺便核对结果。
    auto t0 = std::chrono::steady_clock::now();
    for (int py = 0; py < height; ++py)
        for (int px = 0; px < width; ++px)
            ref[size_t(py) * width + px] = color(escape(x0 + px * step, y0 + py * step));
    double cpu_ms = std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - t0).count();
    size_t diff = 0;
    for (size_t i = 0; i < img.size(); ++i)
        diff += img[i].x != ref[i].x || img[i].y != ref[i].y || img[i].z != ref[i].z;

    printf("%d×%d，grid %u×%u 个 block，每个 block %u×%u 个线程\n",
           width, height, grid.x, grid.y, block.x, block.y);
    printf("GPU %.2f ms，CPU 单线程 %.1f ms，%zu 个像素和 CPU 结果不同\n", gpu_ms, cpu_ms, diff);

    FILE* f = fopen("mandelbrot.ppm", "wb");          // PPM：最简单的图片格式，头部加原始 RGB
    fprintf(f, "P6\n%d %d\n255\n", width, height);
    fwrite(img.data(), sizeof(uchar3), img.size(), f);
    fclose(f);
    CHECK(cudaFree(d_img));
}
