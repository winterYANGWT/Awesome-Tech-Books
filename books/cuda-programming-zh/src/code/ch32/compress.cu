// compress.cu: 用 VMM 分配可压缩的显存，比较三种数据（全零、0–255 循环、随机）的读写带宽。
//   nvcc -O2 -arch=native compress.cu -lcuda -o compress && ./compress
// NVCCFLAGS: -lcuda
#include <cuda.h>
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
#define CU(call)                                                                   \
    do {                                                                           \
        CUresult r_ = (call);                                                      \
        if (r_ != CUDA_SUCCESS) {                                                  \
            const char *name, *msg;                                                \
            cuGetErrorName(r_, &name);                                             \
            cuGetErrorString(r_, &msg);                                            \
            fprintf(stderr, "%s:%d: %s (%s)\n", __FILE__, __LINE__, name, msg);    \
            exit(1);                                                               \
        }                                                                          \
    } while (0)

const size_t BYTES = size_t(1) << 30;

// 写：按 kind 生成数据。0 全零，1 是 0–255 循环，2 是哈希出来的随机数。
__global__ void write_data(int* a, size_t n, int kind) {
    for (size_t i = blockIdx.x * size_t(blockDim.x) + threadIdx.x; i < n; i += size_t(gridDim.x) * blockDim.x) {
        unsigned x = unsigned(i) * 2654435761u;
        x = (x ^ (x >> 13)) * 0x5bd1e995u;
        a[i] = kind == 0 ? 0 : kind == 1 ? int(i & 255) : int(x);
    }
}
// 读：把所有数据异或起来，结果几乎不可能等于那个常数，只是为了不让编译器把读取优化掉。
__global__ void read_data(const int4* a, size_t n, int* out) {
    int s = 0;
    for (size_t i = blockIdx.x * size_t(blockDim.x) + threadIdx.x; i < n; i += size_t(gridDim.x) * blockDim.x) {
        int4 v = a[i];
        s ^= v.x ^ v.y ^ v.z ^ v.w;
    }
    if (s == 0x12345678) *out = s;
}

// ANCHOR: alloc
CUdeviceptr alloc(bool compressible, CUmemGenericAllocationHandle* h) {
    CUmemAllocationProp prop = {};
    prop.type = CU_MEM_ALLOCATION_TYPE_PINNED;
    prop.location.type = CU_MEM_LOCATION_TYPE_DEVICE;
    prop.location.id = 0;
    if (compressible) prop.allocFlags.compressionType = CU_MEM_ALLOCATION_COMP_GENERIC;
    CU(cuMemCreate(h, BYTES, &prop, 0));
    CUmemAllocationProp got;                          // 申请了不一定能得到，要查实际的属性
    CU(cuMemGetAllocationPropertiesFromHandle(&got, *h));
    if (compressible && got.allocFlags.compressionType != CU_MEM_ALLOCATION_COMP_GENERIC)
        printf("（没有得到可压缩内存）\n");
    CUdeviceptr va;
    CU(cuMemAddressReserve(&va, BYTES, 0, 0, 0));
    CU(cuMemMap(va, BYTES, 0, *h, 0));
    CUmemAccessDesc acc = {};
    acc.location = prop.location;
    acc.flags = CU_MEM_ACCESS_FLAGS_PROT_READWRITE;
    CU(cuMemSetAccess(va, BYTES, &acc, 1));
    return va;
}
// ANCHOR_END: alloc

int main() {
    CHECK(cudaFree(0));
    int* out;
    CHECK(cudaMalloc(&out, sizeof(int)));
    cudaEvent_t e0, e1;
    CHECK(cudaEventCreate(&e0));
    CHECK(cudaEventCreate(&e1));
    const char* kinds[] = {"全零", "0–255 循环", "随机"};
    printf("1 GB 显存，每项重复 10 次取平均\n");
    printf("内存      数据          读           写\n");
    for (int comp = 0; comp < 2; ++comp) {
        CUmemGenericAllocationHandle h;
        CUdeviceptr va = alloc(comp, &h);
        for (int kind = 0; kind < 3; ++kind) {
            write_data<<<208, 256>>>((int*)va, BYTES / 4, kind);
            read_data<<<208, 256>>>((const int4*)va, BYTES / 16, out);   // 预热
            float ms_r, ms_w;
            CHECK(cudaEventRecord(e0));
            for (int r = 0; r < 10; ++r) read_data<<<208, 256>>>((const int4*)va, BYTES / 16, out);
            CHECK(cudaEventRecord(e1));
            CHECK(cudaEventSynchronize(e1));
            CHECK(cudaEventElapsedTime(&ms_r, e0, e1));
            CHECK(cudaEventRecord(e0));
            for (int r = 0; r < 10; ++r) write_data<<<208, 256>>>((int*)va, BYTES / 4, kind);
            CHECK(cudaEventRecord(e1));
            CHECK(cudaEventSynchronize(e1));
            CHECK(cudaEventElapsedTime(&ms_w, e0, e1));
            printf("%s  %-12s %7.0f GB/s %7.0f GB/s\n", comp ? "可压缩" : "普通  ", kinds[kind],
                   10 * BYTES / ms_r / 1e6, 10 * BYTES / ms_w / 1e6);
        }
        CU(cuMemUnmap(va, BYTES));
        CU(cuMemAddressFree(va, BYTES));
        CU(cuMemRelease(h));
    }
}
