// halo.cu: 二维热传导，网格按行切成 P 块，第 p 块放在设备 p % 设备数 上，每步和上下邻居交换一行边界（halo）。
// 只有一张卡时，P 块都在这张卡上，可以先把分块和交换的逻辑调对；放到多卡机器上不用改代码。
//   nvcc -O2 -arch=native halo.cu -o halo && ./halo 2 4 8 16
//   ./halo nowait 2 4 8 16        # 去掉块之间的等待，看看会怎样
#include <chrono>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
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

const int NX = 4096, NY = 4096, STEPS = 200;
const float R = 0.2f;
bool g_wait = true;

// 每个线程更新一个格点。in 和 out 各有 rows + 2 行，第 0 行和第 rows + 1 行是 halo，只读不写。
// x 方向是周期边界，在 kernel 里绕回；y 方向的周期边界靠 halo 交换实现。
__global__ void heat_step(int rows, const float* __restrict__ in, float* __restrict__ out) {
    int x = blockIdx.x * blockDim.x + threadIdx.x, y = blockIdx.y + 1;
    if (x >= NX || y > rows) return;
    int xl = x == 0 ? NX - 1 : x - 1, xr = x == NX - 1 ? 0 : x + 1;
    float c = in[y * NX + x];
    out[y * NX + x] = c + R * (in[y * NX + xl] + in[y * NX + xr] + in[(y - 1) * NX + x] +
                               in[(y + 1) * NX + x] - 4 * c);
}

// ANCHOR: part
struct Part {
    int dev, row0, rows;      // 所在设备；负责全局的第 row0 行起的 rows 行
    float* u[2];              // 两个缓冲轮流读写，各有 rows + 2 行
    cudaStream_t stream;      // 在 dev 上创建
    cudaEvent_t done;         // 这一块本步的计算做完了
};
// ANCHOR_END: part

// 把网格切成 np 块分到各设备上，跑 STEPS 步，结果收集到 out，返回每步的毫秒数。
double run(int np, int ndev, const std::vector<float>& init, std::vector<float>& out) {
    // ANCHOR: setup
    std::vector<Part> parts(np);
    for (int p = 0; p < np; ++p) {
        Part& q = parts[p];
        q.dev = p % ndev;
        q.row0 = int(long(NY) * p / np);
        q.rows = int(long(NY) * (p + 1) / np) - q.row0;
        CHECK(cudaSetDevice(q.dev));                  // 之后的分配、stream、event 都属于这个设备
        for (float*& b : q.u) CHECK(cudaMalloc(&b, size_t(q.rows + 2) * NX * sizeof(float)));
        CHECK(cudaStreamCreateWithFlags(&q.stream, cudaStreamNonBlocking));
        CHECK(cudaEventCreateWithFlags(&q.done, cudaEventDisableTiming));
        CHECK(cudaMemcpy(q.u[0] + NX, init.data() + size_t(q.row0) * NX, size_t(q.rows) * NX * sizeof(float),
                         cudaMemcpyHostToDevice));
    }
    // ANCHOR_END: setup
    // ANCHOR: step
    auto step = [&](int k) {
        int cur = k % 2;
        // 1. 每块从上下邻居各拷来一行，放进自己的 halo。先等邻居上一步的计算做完。
        for (int p = 0; p < np; ++p) {
            Part &q = parts[p], &up = parts[(p + np - 1) % np], &down = parts[(p + 1) % np];
            CHECK(cudaSetDevice(q.dev));
            if (g_wait) {
                CHECK(cudaStreamWaitEvent(q.stream, up.done, 0));    // event 可以属于别的设备
                CHECK(cudaStreamWaitEvent(q.stream, down.done, 0));
            }
            CHECK(cudaMemcpyPeerAsync(q.u[cur], q.dev, up.u[cur] + size_t(up.rows) * NX, up.dev,
                                      NX * sizeof(float), q.stream));
            CHECK(cudaMemcpyPeerAsync(q.u[cur] + size_t(q.rows + 1) * NX, q.dev, down.u[cur] + NX, down.dev,
                                      NX * sizeof(float), q.stream));
        }
        // 2. 各块在自己的设备、自己的 stream 上计算，记下 done。
        for (Part& q : parts) {
            CHECK(cudaSetDevice(q.dev));
            heat_step<<<dim3(NX / 256, q.rows), 256, 0, q.stream>>>(q.rows, q.u[cur], q.u[1 - cur]);
            CHECK(cudaGetLastError());
            CHECK(cudaEventRecord(q.done, q.stream));
        }
    };
    // ANCHOR_END: step
    auto sync_all = [&] {
        for (int d = 0; d < ndev; ++d) {
            CHECK(cudaSetDevice(d));
            CHECK(cudaDeviceSynchronize());
        }
    };
    step(0), step(1);                                 // 预热，顺便让结果从第 0 个缓冲开始
    sync_all();
    auto t0 = std::chrono::steady_clock::now();
    for (int k = 2; k < STEPS + 2; ++k) step(k);
    sync_all();
    double ms = std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - t0).count();
    for (Part& q : parts) {                           // 统一地址空间里，指针本身就说明了它在哪个设备上
        CHECK(cudaMemcpy(out.data() + size_t(q.row0) * NX, q.u[(STEPS + 2) % 2] + NX,
                         size_t(q.rows) * NX * sizeof(float), cudaMemcpyDefault));
        CHECK(cudaSetDevice(q.dev));
        for (float* b : q.u) CHECK(cudaFree(b));
        CHECK(cudaStreamDestroy(q.stream));
        CHECK(cudaEventDestroy(q.done));
    }
    return ms / STEPS;
}

int main(int argc, char** argv) {
    int ndev = 0;
    CHECK(cudaGetDeviceCount(&ndev));
    // ANCHOR: peer
    // 能直接访问的设备两两打开 peer access，peer 拷贝就能走卡间直连；打不开也能拷，只是经主机内存中转。
    int enabled = 0;
    for (int a = 0; a < ndev; ++a)
        for (int b = 0; b < ndev; ++b) {
            int can = 0;
            if (a != b) CHECK(cudaDeviceCanAccessPeer(&can, a, b));
            if (!can) continue;
            CHECK(cudaSetDevice(a));
            CHECK(cudaDeviceEnablePeerAccess(b, 0));
            ++enabled;
        }
    // ANCHOR_END: peer
    int first = 1;
    if (argc > 1 && !strcmp(argv[1], "nowait")) g_wait = false, first = 2;
    printf("%d 个设备，打开了 %d 对 peer access；网格 %d×%d，%d 步%s\n", ndev, enabled, NX, NY, STEPS,
           g_wait ? "" : "，块之间不等待");
    std::vector<float> init(size_t(NX) * NY), ref(init.size()), out(init.size());
    for (int y = 0; y < NY; ++y)
        for (int x = 0; x < NX; ++x) init[size_t(y) * NX + x] = float((x * 7 + y * 13) % 101);
    double ms1 = run(1, ndev, init, ref);            // 只有一块时只用一个 stream，不存在块之间的同步问题
    printf("  块数    每步       相对 1 块   和 1 块结果的最大差\n");
    printf("%6d  %7.3f ms\n", 1, ms1);
    for (int i = first; i < argc; ++i) {
        int np = atoi(argv[i]);
        if (np == 1) continue;
        double ms = run(np, ndev, init, out);
        float diff = 0;
        for (size_t k = 0; k < out.size(); ++k) diff = fmaxf(diff, fabsf(out[k] - ref[k]));
        printf("%6d  %7.3f ms  %8.2f   %g\n", np, ms, ms / ms1, diff);
    }
}
