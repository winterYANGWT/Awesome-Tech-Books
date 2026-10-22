// pitch.cu: 二维数组的行对齐（pitch），以及只拷贝一幅图像中的一块矩形区域。
//   nvcc -O2 -arch=native pitch.cu -o pitch && ./pitch
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

// ANCHOR: kernel
// 二维 kernel 用 pitch（每行实际占的字节数）定位一行的开头。
__global__ void scale(float* img, size_t pitch, int width, int height, float k) {
    int x = blockIdx.x * blockDim.x + threadIdx.x;
    int y = blockIdx.y * blockDim.y + threadIdx.y;
    if (x >= width || y >= height) return;
    float* row = reinterpret_cast<float*>(reinterpret_cast<char*>(img) + y * pitch);
    row[x] *= k;
}
// ANCHOR_END: kernel

int main() {
    const int W = 1000, H = 800;               // 主机上一幅 1000×800 的 float 图像，紧密排列
    std::vector<float> host(W * H);
    for (int i = 0; i < W * H; ++i) host[i] = float(i);

    // ANCHOR: alloc
    // 显存里只要中间 300×200 的一块。cudaMallocPitch 会把每行补齐到合适的字节数。
    const int w = 300, h = 200, x0 = 350, y0 = 300;
    float* dev;
    size_t pitch;
    CHECK(cudaMallocPitch(&dev, &pitch, w * sizeof(float), h));
    printf("每行 %zu 字节的数据，cudaMallocPitch 给出的 pitch 是 %zu 字节\n", w * sizeof(float), pitch);

    // 从主机图像的 (x0, y0) 处拷一块 w×h 的矩形进来：源和目标的行宽不同，按行拷。
    CHECK(cudaMemcpy2D(dev, pitch,                                 // 目标及其 pitch
                       host.data() + y0 * W + x0, W * sizeof(float),  // 源及其行宽
                       w * sizeof(float), h,                       // 每行拷多少字节、拷几行
                       cudaMemcpyHostToDevice));
    // ANCHOR_END: alloc

    dim3 block(32, 8), grid((w + 31) / 32, (h + 7) / 8);
    scale<<<grid, block>>>(dev, pitch, w, h, 2.0f);
    CHECK(cudaGetLastError());

    // 再拷回原处，只覆盖那块矩形。
    CHECK(cudaMemcpy2D(host.data() + y0 * W + x0, W * sizeof(float), dev, pitch,
                       w * sizeof(float), h, cudaMemcpyDeviceToHost));
    int bad = 0;
    for (int y = 0; y < H; ++y)
        for (int x = 0; x < W; ++x) {
            bool inside = x >= x0 && x < x0 + w && y >= y0 && y < y0 + h;
            float expect = float(y * W + x) * (inside ? 2.0f : 1.0f);
            bad += host[y * W + x] != expect;
        }
    printf("%d 个像素不对\n", bad);
    CHECK(cudaFree(dev));
}
