// graphmem.cu: 捕获时调用 cudaMallocAsync/cudaFreeAsync，它们会变成 graph 里的内存节点。
// 比较这样的 graph 和使用预先分配好的缓冲区的 graph。
//   nvcc -O2 -arch=native graphmem.cu -o graphmem && ./graphmem
#include <chrono>
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

constexpr size_t BYTES = size_t(64) << 20;
constexpr int ROUNDS = 200;

__global__ void use(float* tmp, float* out, size_t n) {
    float s = 0;
    for (size_t i = blockIdx.x * size_t(blockDim.x) + threadIdx.x; i < n; i += size_t(gridDim.x) * blockDim.x) {
        tmp[i] = float(i);
        s += tmp[i];
    }
    if (s == -1.0f) *out = s;
}

double now_us() {
    using namespace std::chrono;
    return duration<double, std::micro>(steady_clock::now().time_since_epoch()).count();
}

unsigned long long graph_mem_mb(cudaGraphMemAttributeType attr) {
    unsigned long long v;
    CHECK(cudaDeviceGetGraphMemAttribute(0, attr, &v));
    return v >> 20;
}

int main() {
    const size_t n = BYTES / sizeof(float);
    float *out, *fixed;
    CHECK(cudaMalloc(&out, sizeof(float)));
    CHECK(cudaMalloc(&fixed, BYTES));
    cudaStream_t s;
    CHECK(cudaStreamCreateWithFlags(&s, cudaStreamNonBlocking));

    // ANCHOR: capture
    // 临时缓冲区的分配和释放也录进 graph。
    cudaGraph_t g_alloc;
    CHECK(cudaStreamBeginCapture(s, cudaStreamCaptureModeGlobal));
    float* tmp;
    CHECK(cudaMallocAsync(&tmp, BYTES, s));      // 变成一个内存分配节点
    use<<<26 * 6, 256, 0, s>>>(tmp, out, n);
    CHECK(cudaFreeAsync(tmp, s));                // 变成一个内存释放节点
    CHECK(cudaStreamEndCapture(s, &g_alloc));
    // ANCHOR_END: capture

    cudaGraph_t g_fixed;
    CHECK(cudaStreamBeginCapture(s, cudaStreamCaptureModeGlobal));
    use<<<26 * 6, 256, 0, s>>>(fixed, out, n);
    CHECK(cudaStreamEndCapture(s, &g_fixed));

    size_t nodes;
    CHECK(cudaGraphGetNodes(g_alloc, nullptr, &nodes));
    printf("带内存节点的 graph 有 %zu 个节点\n", nodes);
    cudaGraphExec_t e_alloc, e_fixed;
    CHECK(cudaGraphInstantiate(&e_alloc, g_alloc, 0));
    CHECK(cudaGraphInstantiate(&e_fixed, g_fixed, 0));
    printf("实例化之后，为 graph 保留的显存 %llu MB\n", graph_mem_mb(cudaGraphMemAttrReservedMemCurrent));
    CHECK(cudaGraphLaunch(e_alloc, s));
    CHECK(cudaStreamSynchronize(s));
    printf("第一次启动之后，为 graph 保留的显存 %llu MB\n\n", graph_mem_mb(cudaGraphMemAttrReservedMemCurrent));

    for (cudaGraphExec_t e : {e_fixed, e_alloc}) {
        CHECK(cudaGraphLaunch(e, s));
        CHECK(cudaStreamSynchronize(s));
        double t0 = now_us();
        for (int r = 0; r < ROUNDS; ++r) {
            CHECK(cudaGraphLaunch(e, s));
            CHECK(cudaStreamSynchronize(s));     // 每轮都同步
        }
        printf("%7.1f us  %s\n", (now_us() - t0) / ROUNDS,
               e == e_alloc ? "graph 里分配和释放临时缓冲区" : "graph 用预先分配好的缓冲区");
    }

    CHECK(cudaGraphExecDestroy(e_alloc));
    CHECK(cudaGraphExecDestroy(e_fixed));
    CHECK(cudaGraphDestroy(g_alloc));
    CHECK(cudaGraphDestroy(g_fixed));
    CHECK(cudaDeviceGraphMemTrim(0));            // 把 graph 不再需要的显存还给系统
    printf("\n销毁 graph 并 trim 之后，为 graph 保留的显存 %llu MB\n", graph_mem_mb(cudaGraphMemAttrReservedMemCurrent));
    CHECK(cudaFree(fixed));
    CHECK(cudaFree(out));
}
