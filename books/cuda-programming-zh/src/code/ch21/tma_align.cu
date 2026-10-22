// tma_align.cu: 用 TMA 从 1024×1024 的 float 矩阵里取一个 32×32 的块，块的左上角坐标从命令行给出。
// 非法指令会让整个 context 作废，所以每个坐标单独运行一次。
//   nvcc -O2 -arch=native tma_align.cu -o tma_align -lcuda && ./tma_align 4 0 && ./tma_align 1 0
// NVCCFLAGS: -lcuda
#include <cuda.h>
#include <cuda/barrier>
#include <cuda/ptx>
#include <cstdio>
#include <cstdlib>

using block_barrier = cuda::barrier<cuda::thread_scope_block>;

__global__ void load_box(const __grid_constant__ CUtensorMap map, int x, int y, float* out) {
    __shared__ alignas(128) float tile[32 * 32];
    __shared__ alignas(block_barrier) unsigned char bar_storage[sizeof(block_barrier)];
    block_barrier& bar = *reinterpret_cast<block_barrier*>(bar_storage);
    if (threadIdx.x == 0) init(&bar, blockDim.x);
    __syncthreads();
    block_barrier::arrival_token token;
    if (threadIdx.x == 0) {
        int32_t coords[2] = {x, y};
        cuda::ptx::cp_async_bulk_tensor(cuda::ptx::space_cluster, cuda::ptx::space_global, tile, &map,
                                        coords, cuda::device::barrier_native_handle(bar));
        token = cuda::device::barrier_arrive_tx(bar, 1, sizeof(tile));
    } else {
        token = bar.arrive();
    }
    bar.wait(std::move(token));
    out[threadIdx.x] = tile[threadIdx.x];
}

int main(int argc, char** argv) {
    if (argc < 3) { fprintf(stderr, "usage: %s x y\n", argv[0]); return 2; }
    int x = atoi(argv[1]), y = atoi(argv[2]);
    const int n = 1024;
    float *in, *out;
    cudaMalloc(&in, n * n * sizeof(float));
    cudaMalloc(&out, 32 * sizeof(float));
    CUtensorMap map;
    cuuint64_t dims[2] = {n, n}, strides[1] = {n * sizeof(float)};
    cuuint32_t box[2] = {32, 32}, elem_strides[2] = {1, 1};
    CUresult r = cuTensorMapEncodeTiled(&map, CU_TENSOR_MAP_DATA_TYPE_FLOAT32, 2, in, dims, strides, box,
                                        elem_strides, CU_TENSOR_MAP_INTERLEAVE_NONE, CU_TENSOR_MAP_SWIZZLE_NONE,
                                        CU_TENSOR_MAP_L2_PROMOTION_NONE, CU_TENSOR_MAP_FLOAT_OOB_FILL_NONE);
    load_box<<<1, 128>>>(map, x, y, out);
    cudaError_t e = cudaDeviceSynchronize();
    printf("块的左上角 (%d, %d)：描述符 %s，运行 %s\n", x, y, r == CUDA_SUCCESS ? "正常" : "出错",
           cudaGetErrorName(e));
    return e == cudaSuccess ? 0 : 1;
}
