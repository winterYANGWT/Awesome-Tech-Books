// resident.cu: 不同的 block 大小下，每个 SM 最多能同时驻留几个 block、多少个线程。
//   nvcc -O2 -arch=native resident.cu -o resident && ./resident
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

__global__ void brighten(const float* in, float* out, int width, int height) {
    int x = blockIdx.x * blockDim.x + threadIdx.x;
    int y = blockIdx.y * blockDim.y + threadIdx.y;
    if (x < width && y < height) {
        int i = y * width + x;
        out[i] = in[i] * 1.2f + 0.05f;
    }
}

int main() {
    cudaDeviceProp prop;
    CHECK(cudaGetDeviceProperties(&prop, 0));
    printf("每个 SM 最多 %d 个 block、%d 个线程\n\n", prop.maxBlocksPerMultiProcessor,
           prop.maxThreadsPerMultiProcessor);
    printf("block 大小  每个 SM 驻留的 block  驻留的线程\n");
    for (int threads : {32, 64, 128, 256, 512, 1024}) {
        int blocks;
        // ANCHOR: query
        CHECK(cudaOccupancyMaxActiveBlocksPerMultiprocessor(&blocks, brighten, threads, 0));
        // ANCHOR_END: query
        printf("%8d  %14d  %16d\n", threads, blocks, blocks * threads);
    }
}
