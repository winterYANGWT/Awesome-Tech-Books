// vmm.cu: 虚拟内存管理（VMM）API 把“地址”和“物理内存”分开管理。
// 实验一：一个不断变大的数组，比较“分配新的、拷过去、释放旧的”和“在预留的地址后面接着映射”。
// 实验二：同一块物理内存映射到两个相邻的地址，做成一个读写时不用处理绕回的环形缓冲。
//   nvcc -O2 -arch=native vmm.cu -lcuda -o vmm && ./vmm
// NVCCFLAGS: -lcuda
#include <cuda.h>
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

static double now_ms() {
    return std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now().time_since_epoch()).count();
}

__global__ void fill(int* a, size_t from, size_t to) {
    for (size_t i = from + blockIdx.x * size_t(blockDim.x) + threadIdx.x; i < to; i += size_t(gridDim.x) * blockDim.x)
        a[i] = int(i);
}
__global__ void count_wrong(const int* a, size_t n, unsigned long long* wrong) {
    for (size_t i = blockIdx.x * size_t(blockDim.x) + threadIdx.x; i < n; i += size_t(gridDim.x) * blockDim.x)
        if (a[i] != int(i)) atomicAdd(wrong, 1ull);
}

unsigned long long check(const int* a, size_t n) {
    unsigned long long *d, h;
    CHECK(cudaMalloc(&d, sizeof h));
    CHECK(cudaMemset(d, 0, sizeof h));
    count_wrong<<<1024, 256>>>(a, n, d);
    CHECK(cudaMemcpy(&h, d, sizeof h, cudaMemcpyDeviceToHost));
    CHECK(cudaFree(d));
    return h;
}

// ANCHOR: prop
CUmemAllocationProp device_prop() {                   // 物理内存放在设备 0 的显存里
    CUmemAllocationProp prop = {};
    prop.type = CU_MEM_ALLOCATION_TYPE_PINNED;
    prop.location.type = CU_MEM_LOCATION_TYPE_DEVICE;
    prop.location.id = 0;
    return prop;
}
void allow_rw(CUdeviceptr va, size_t size) {          // 映射以后还要单独授予访问权限
    CUmemAccessDesc acc = {};
    acc.location.type = CU_MEM_LOCATION_TYPE_DEVICE;
    acc.location.id = 0;
    acc.flags = CU_MEM_ACCESS_FLAGS_PROT_READWRITE;
    CU(cuMemSetAccess(va, size, &acc, 1));
}
// ANCHOR_END: prop

// ANCHOR: grow
// 先预留一大段虚拟地址，需要更多内存时在已映射部分的后面再映射一块物理内存。
// 地址始终不变，已有的数据不用搬。
struct GrowableBuffer {
    CUdeviceptr base = 0;
    size_t reserved = 0, mapped = 0, chunk;
    std::vector<CUmemGenericAllocationHandle> handles;
    GrowableBuffer(size_t max_bytes, size_t chunk_bytes) : reserved(max_bytes), chunk(chunk_bytes) {
        CU(cuMemAddressReserve(&base, reserved, 0, 0, 0));
    }
    void grow_to(size_t bytes) {
        CUmemAllocationProp prop = device_prop();
        while (mapped < bytes) {
            CUmemGenericAllocationHandle h;
            CU(cuMemCreate(&h, chunk, &prop, 0));     // 一块物理内存
            CU(cuMemMap(base + mapped, chunk, 0, h, 0));   // 接在已映射部分的后面
            allow_rw(base + mapped, chunk);
            handles.push_back(h);
            mapped += chunk;
        }
    }
    ~GrowableBuffer() {
        for (size_t k = 0; k < handles.size(); ++k) {
            CU(cuMemUnmap(base + k * chunk, chunk));
            CU(cuMemRelease(handles[k]));
        }
        CU(cuMemAddressFree(base, reserved));
    }
};
// ANCHOR_END: grow

int main() {
    CHECK(cudaFree(0));                               // 让 Runtime API 建立主上下文，Driver API 也用它
    CUdevice dev;
    CU(cuDeviceGet(&dev, 0));
    // ANCHOR: attrs
    struct { const char* name; CUdevice_attribute attr; } attrs[] = {
        {"VMM", CU_DEVICE_ATTRIBUTE_VIRTUAL_MEMORY_MANAGEMENT_SUPPORTED},
        {"导出为 POSIX 文件描述符", CU_DEVICE_ATTRIBUTE_HANDLE_TYPE_POSIX_FILE_DESCRIPTOR_SUPPORTED},
        {"multicast", CU_DEVICE_ATTRIBUTE_MULTICAST_SUPPORTED},
        {"压缩内存", CU_DEVICE_ATTRIBUTE_GENERIC_COMPRESSION_SUPPORTED},
        {"VMM 分配主机 NUMA 内存", CU_DEVICE_ATTRIBUTE_HOST_NUMA_VIRTUAL_MEMORY_MANAGEMENT_SUPPORTED},
    };
    for (auto& a : attrs) {
        int v;
        CU(cuDeviceGetAttribute(&v, a.attr, dev));
        printf("%s：%s\n", a.name, v ? "支持" : "不支持");
    }
    CUmemAllocationProp prop = device_prop();
    size_t gmin, grec;
    CU(cuMemGetAllocationGranularity(&gmin, &prop, CU_MEM_ALLOC_GRANULARITY_MINIMUM));
    CU(cuMemGetAllocationGranularity(&grec, &prop, CU_MEM_ALLOC_GRANULARITY_RECOMMENDED));
    printf("分配粒度：最小 %zu KB，推荐 %zu KB\n", gmin >> 10, grec >> 10);
    // ANCHOR_END: attrs

    // 实验一：数组每次长 64 MB，一直长到 2 GB，每次只填新长出来的部分。
    const size_t step = 64 << 20, max_bytes = size_t(2) << 30;
    printf("\n数组从 64 MB 长到 2 GB，每次长 64 MB：\n");
    // ANCHOR: realloc
    {
        double t0 = now_ms();
        int* a = nullptr;
        for (size_t bytes = step, old = 0; bytes <= max_bytes; old = bytes, bytes += step) {
            int* b;
            CHECK(cudaMalloc(&b, bytes));
            if (a) CHECK(cudaMemcpy(b, a, old, cudaMemcpyDeviceToDevice));   // 把旧的内容搬过去
            CHECK(cudaFree(a));
            a = b;
            fill<<<1024, 256>>>(a, old / 4, bytes / 4);
        }
        CHECK(cudaDeviceSynchronize());
        double ms = now_ms() - t0;
        printf("  重新分配并拷贝：%8.1f ms，错误 %llu 个\n", ms, check(a, max_bytes / 4));
        CHECK(cudaFree(a));
    }
    // ANCHOR_END: realloc
    // ANCHOR: vmm
    {
        double t0 = now_ms();
        GrowableBuffer buf(max_bytes, step);
        for (size_t bytes = step, old = 0; bytes <= max_bytes; old = bytes, bytes += step) {
            buf.grow_to(bytes);
            fill<<<1024, 256>>>((int*)buf.base, old / 4, bytes / 4);
        }
        CHECK(cudaDeviceSynchronize());
        double ms = now_ms() - t0;
        printf("  VMM 接着映射：  %8.1f ms，错误 %llu 个\n", ms, check((int*)buf.base, max_bytes / 4));
    }
    // ANCHOR_END: vmm

    // 实验二：同一块物理内存映射到相邻的两段地址。
    // ANCHOR: ring
    const size_t ring = gmin;                         // 环形缓冲的大小取一个分配粒度
    const size_t n = ring / sizeof(int);
    CUmemGenericAllocationHandle h;
    CU(cuMemCreate(&h, ring, &prop, 0));
    CUdeviceptr va;
    CU(cuMemAddressReserve(&va, 2 * ring, 0, 0, 0));
    CU(cuMemMap(va, ring, 0, h, 0));                  // 前一半
    CU(cuMemMap(va + ring, ring, 0, h, 0));           // 后一半是同一块物理内存
    allow_rw(va, 2 * ring);
    fill<<<64, 256>>>((int*)va, 0, n);               // 只通过前一半写入 0, 1, ..., n-1
    int tail[8];
    CHECK(cudaMemcpy(tail, (int*)va + n - 4, sizeof tail, cudaMemcpyDeviceToHost));   // 跨过末尾读 8 个
    printf("\n环形缓冲 %zu 个 int，从第 %zu 个起连续读 8 个：", n, n - 4);
    for (int v : tail) printf(" %d", v);
    printf("\n");
    CU(cuMemUnmap(va, 2 * ring));
    CU(cuMemAddressFree(va, 2 * ring));
    CU(cuMemRelease(h));
    // ANCHOR_END: ring
}
