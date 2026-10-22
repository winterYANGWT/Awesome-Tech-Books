// log.cu: 两个常见的参数错误。返回的错误码只说明“参数不对”，设置 CUDA_LOG_FILE 以后驱动会说清楚哪里不对。
//   nvcc -O2 -arch=native log.cu -o log && ./log && CUDA_LOG_FILE=stdout ./log
#include <cstdio>

__global__ void k(float* a) { a[threadIdx.x] = 0; }

int main() {
    setvbuf(stdout, nullptr, _IONBF, 0);              // 和驱动的日志按顺序交错输出
    float* a;
    cudaMalloc(&a, 4096);
    k<<<1, 2048>>>(a);                                // 每个 block 最多 1024 个线程
    printf("启动 2048 个线程的 block：%s\n", cudaGetErrorName(cudaGetLastError()));
    k<<<1, 256, 200 * 1024>>>(a);                     // 动态 shared memory 超过上限
    printf("申请 200 KB 动态 shared memory：%s\n", cudaGetErrorName(cudaGetLastError()));
}
