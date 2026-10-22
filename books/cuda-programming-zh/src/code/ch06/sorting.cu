// sorting.cu: 4 种类型的数据各走一段不同的代码。类型随机排列和按类型排好序，速度差多少。
//   nvcc -O2 -arch=native sorting.cu -o sorting && ./sorting
#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <random>
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

struct Item {
    int type;      // 0 到 3
    float value;
};

// ANCHOR: kernel
// 每种类型一段不同的计算，计算量大致相当。
__global__ void process(const Item* items, float* out, int n) {
    int i = blockIdx.x * blockDim.x + threadIdx.x;
    if (i >= n) return;
    float v = items[i].value;
    switch (items[i].type) {
    case 0: for (int k = 0; k < 400; ++k) v = sinf(v) + 0.5f; break;
    case 1: for (int k = 0; k < 400; ++k) v = cosf(v) * 0.9f; break;
    case 2: for (int k = 0; k < 400; ++k) v = sqrtf(v * v + 1.0f) - 0.7f; break;
    default: for (int k = 0; k < 400; ++k) v = expf(-v * v); break;
    }
    out[i] = v;
}
// ANCHOR_END: kernel

int main() {
    const int n = 1 << 22;
    std::vector<Item> items(n);
    std::mt19937 rng(42);
    for (auto& it : items) it.type = rng() % 4, it.value = float(rng() % 1000) / 1000.0f;

    Item* d_items;
    float* d_out;
    CHECK(cudaMalloc(&d_items, n * sizeof(Item)));
    CHECK(cudaMalloc(&d_out, n * sizeof(float)));
    cudaEvent_t start, stop;
    CHECK(cudaEventCreate(&start));
    CHECK(cudaEventCreate(&stop));

    auto run = [&](const char* label) {
        CHECK(cudaMemcpy(d_items, items.data(), n * sizeof(Item), cudaMemcpyHostToDevice));
        process<<<(n + 255) / 256, 256>>>(d_items, d_out, n);   // 预热
        CHECK(cudaGetLastError());
        float best = 1e30f;
        for (int r = 0; r < 5; ++r) {
            CHECK(cudaEventRecord(start));
            process<<<(n + 255) / 256, 256>>>(d_items, d_out, n);
            CHECK(cudaEventRecord(stop));
            CHECK(cudaEventSynchronize(stop));
            float ms;
            CHECK(cudaEventElapsedTime(&ms, start, stop));
            if (ms < best) best = ms;
        }
        printf("%-12s %6.2f ms\n", label, best);
    };

    run("随机排列");
    // ANCHOR: sort
    std::stable_sort(items.begin(), items.end(),
                     [](const Item& a, const Item& b) { return a.type < b.type; });
    // ANCHOR_END: sort
    run("按类型排序");

    CHECK(cudaFree(d_items));
    CHECK(cudaFree(d_out));
}
