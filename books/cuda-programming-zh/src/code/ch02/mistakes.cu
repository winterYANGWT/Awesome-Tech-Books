// mistakes.cu: 三个常见错误，看它们实际表现成什么样。
//   nvcc -O2 -arch=native mistakes.cu -o mistakes
//   ./mistakes deref | config | kind
#include <cstdio>
#include <cstdlib>
#include <cstring>

#define CHECK(call)                                                        \
    do {                                                                   \
        cudaError_t err_ = (call);                                         \
        if (err_ != cudaSuccess) {                                         \
            fprintf(stderr, "%s:%d: %s (%s)\n", __FILE__, __LINE__,        \
                    cudaGetErrorName(err_), cudaGetErrorString(err_));     \
            exit(1);                                                       \
        }                                                                  \
    } while (0)

__global__ void fill(float* p, int n, float v) {
    int i = blockIdx.x * blockDim.x + threadIdx.x;
    if (i < n) p[i] = v;
}

int main(int argc, char** argv) {
    const char* what = argc > 1 ? argv[1] : "";
    const int n = 4096;
    float* d;
    CHECK(cudaMalloc(&d, n * sizeof(float)));
    CHECK(cudaMemset(d, 0, n * sizeof(float)));
    float h[n];

    if (!strcmp(what, "deref")) {
        // ANCHOR: deref
        // 错误 1：在主机代码里直接读显存指针。
        printf("d = %p\n", (void*)d);
        fflush(stdout);                     // 下一行会让程序崩溃，先把上一行输出
        printf("d[0] = %f\n", d[0]);
        // ANCHOR_END: deref
    } else if (!strcmp(what, "config")) {
        // ANCHOR: config
        // 错误 2：一个 block 开 2048 个线程，超过了上限 1024。不检查，也没有任何提示。
        fill<<<n / 2048, 2048>>>(d, n, 1.0f);
        CHECK(cudaMemcpy(h, d, sizeof(h), cudaMemcpyDeviceToHost));
        printf("不检查：h[0] = %f\n", h[0]);
        fflush(stdout);
        // 同样的启动，后面接一次 cudaGetLastError。
        fill<<<n / 2048, 2048>>>(d, n, 1.0f);
        CHECK(cudaGetLastError());
        // ANCHOR_END: config
    } else if (!strcmp(what, "kind")) {
        // ANCHOR: kind
        // 错误 3：拷贝方向写反，源是主机数组，却声明成从显存拷出。
        for (int i = 0; i < n; ++i) h[i] = 2.0f;
        CHECK(cudaMemcpy(d, h, sizeof(h), cudaMemcpyDeviceToHost));
        float back[n];
        CHECK(cudaMemcpy(back, d, sizeof(back), cudaMemcpyDeviceToHost));
        printf("拷贝返回成功，d[0] = %f\n", back[0]);
        // ANCHOR_END: kind
    } else {
        fprintf(stderr, "usage: %s deref | config | kind\n", argv[0]);
        return 2;
    }
    CHECK(cudaFree(d));
}
