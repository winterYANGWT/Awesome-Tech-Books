// ipc_limits.cu: cudaIpcGetMemHandle 的几个限制。
//   nvcc -O2 -arch=native ipc_limits.cu -o ipc_limits && ./ipc_limits
#include <cstdio>
#include <cstring>

int main() {
    // ANCHOR: limits
    int* a;
    cudaMalloc(&a, 1 << 20);
    cudaIpcMemHandle_t h, h_mid;
    cudaIpcGetMemHandle(&h, a);
    cudaError_t e = cudaIpcGetMemHandle(&h_mid, a + 1000);   // 指向分配中间的指针
    printf("对 a + 1000 取句柄：%s，和 a 的句柄%s\n", cudaGetErrorName(e),
           memcmp(&h, &h_mid, sizeof h) ? "不同" : "完全相同");
    void* p;
    e = cudaIpcOpenMemHandle(&p, h, cudaIpcMemLazyEnablePeerAccess);
    printf("在分配它的进程里打开：%s\n", cudaGetErrorName(e));
    int *m, *q;
    cudaMallocManaged(&m, 4096);
    printf("托管内存取句柄：%s\n", cudaGetErrorName(cudaIpcGetMemHandle(&h, m)));
    cudaMallocAsync(&q, 4096, 0);                     // 默认内存池里的分配
    cudaDeviceSynchronize();
    printf("cudaMallocAsync 的内存取句柄：%s\n", cudaGetErrorName(cudaIpcGetMemHandle(&h, q)));
    // ANCHOR_END: limits
}
