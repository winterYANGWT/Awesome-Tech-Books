// entry.cu: 不链接 libcuda，通过 Runtime API 按 CUDA 版本取得 Driver API 的函数指针。
// 同一个函数名，按不同的版本去取，拿到的可能是参数不同的另一个函数。
//   nvcc -O2 entry.cu -o entry && ./entry
#include <cuda.h>
#include <cudaTypedefs.h>                            // PFN_ 开头的函数指针类型
#include <dlfcn.h>
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

// 在 libcuda 导出的同名函数的各个版本里，找出和 p 相同的那个。
const char* which(void* lib, const char* name, void* p) {
    static char buf[64];
    const char* suffix[] = {"", "_v2", "_v3", "_v4", "_ptsz", "_v2_ptsz"};
    for (const char* s : suffix) {
        snprintf(buf, sizeof buf, "%s%s", name, s);
        if (dlsym(lib, buf) == p) return buf;
    }
    return "（不是导出的符号）";
}

// ANCHOR: query
void query(void* lib, const char* name, int version, unsigned long long flags = cudaEnableDefault) {
    void* p = nullptr;
    cudaDriverEntryPointQueryResult status;
    CHECK(cudaGetDriverEntryPointByVersion(name, &p, version, flags, &status));
    printf("  %-18s 按 %2d.%d 取%s：", name, version / 1000, version % 1000 / 10,
           flags == cudaEnablePerThreadDefaultStream ? "（每线程默认 stream）" : "");
    if (status == cudaDriverEntryPointSuccess) printf("%s\n", which(lib, name, p));
    else if (status == cudaDriverEntryPointVersionNotSufficent) printf("这个版本还没有这个函数\n");
    else printf("找不到\n");
}
// ANCHOR_END: query

int main() {
    int drv, rt;
    CHECK(cudaDriverGetVersion(&drv));
    CHECK(cudaRuntimeGetVersion(&rt));
    printf("驱动支持到 CUDA %d.%d，程序用的运行时是 CUDA %d.%d\n", drv / 1000, drv % 1000 / 10, rt / 1000,
           rt % 1000 / 10);
    void* lib = dlopen("libcuda.so.1", RTLD_NOW);     // 只用来比对符号，实际调用不需要它
    // ANCHOR: run
    for (int v : {11000, 12000, 12050, 13000}) query(lib, "cuCtxCreate", v);
    query(lib, "cuMemcpyAsync", 13000);
    query(lib, "cuMemcpyAsync", 13000, cudaEnablePerThreadDefaultStream);
    query(lib, "cuLibraryLoadData", 11080);
    query(lib, "cuLibraryLoadData", 12000);
    // ANCHOR_END: run

    // ANCHOR: call
    // 取到的指针要转换成对应版本的函数类型再调用。类型名末尾的数字是这个参数形式出现的版本，
    // cuDeviceGetName 从 CUDA 2.0 起就没变过。
    void* p;
    cudaDriverEntryPointQueryResult status;
    CHECK(cudaGetDriverEntryPointByVersion("cuDeviceGetName", &p, 12000, cudaEnableDefault, &status));
    auto get_name = (PFN_cuDeviceGetName_v2000)p;
    char name[256];
    CUresult r = get_name(name, sizeof name, 0);
    printf("\n通过函数指针调用 cuDeviceGetName：%d，%s\n", int(r), name);
    // ANCHOR_END: call
}
