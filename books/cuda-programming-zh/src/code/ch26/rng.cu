// rng.cu: 用 cuRAND 生成大量随机数：主机端 API 的几种生成器、和 CPU 比较；
// 再看在 kernel 里自己生成时，初始化每个线程的状态要多久。最后用这些随机数估算 π。
//   nvcc -O2 -arch=native rng.cu -o rng -lcurand && ./rng
// NVCCFLAGS: -lcurand
#include <curand.h>
#include <curand_kernel.h>
#include <chrono>
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

#define CHECK_RAND(call)                                                   \
    do {                                                                   \
        curandStatus_t st_ = (call);                                       \
        if (st_ != CURAND_STATUS_SUCCESS) {                                \
            fprintf(stderr, "%s:%d: cuRAND 错误 %d\n", __FILE__, __LINE__,  \
                    int(st_));                                             \
            exit(1);                                                       \
        }                                                                  \
    } while (0)

constexpr size_t N = size_t(256) << 20;      // 2.5 亿多个 float

double now_ms() {
    using namespace std::chrono;
    return duration<double, std::milli>(steady_clock::now().time_since_epoch()).count();
}

// 用一对对随机数 (x, y) 估算 π：落在单位圆内的比例约为 π/4。
__global__ void count_inside(const float* xy, size_t pairs, unsigned long long* inside) {
    unsigned long long c = 0;
    for (size_t i = blockIdx.x * size_t(blockDim.x) + threadIdx.x; i < pairs; i += size_t(gridDim.x) * blockDim.x) {
        float x = xy[2 * i], y = xy[2 * i + 1];
        c += x * x + y * y <= 1.0f;
    }
    atomicAdd(inside, c);
}

// ANCHOR: device_api
// 设备端 API：每个线程有自己的生成器状态，在 kernel 里边生成边用。
template <class State>
__global__ void pi_device(size_t pairs_per_thread, unsigned long long seed, unsigned long long* inside) {
    unsigned id = blockIdx.x * blockDim.x + threadIdx.x;
    State st;
    curand_init(seed, id, 0, &st);           // 同一个种子，每个线程取一条不同的子序列
    unsigned long long c = 0;
    for (size_t k = 0; k < pairs_per_thread; ++k) {
        float x = curand_uniform(&st), y = curand_uniform(&st);
        c += x * x + y * y <= 1.0f;
    }
    atomicAdd(inside, c);
}

// Philox 每次内部产生 4 个数：用 curand_uniform4 一次全部拿走，一次就是两对。
__global__ void pi_philox4(size_t pairs_per_thread, unsigned long long seed, unsigned long long* inside) {
    unsigned id = blockIdx.x * blockDim.x + threadIdx.x;
    curandStatePhilox4_32_10_t st;
    curand_init(seed, id, 0, &st);
    unsigned long long c = 0;
    for (size_t k = 0; k < pairs_per_thread; k += 2) {
        float4 r = curand_uniform4(&st);
        c += (r.x * r.x + r.y * r.y <= 1.0f) + (r.z * r.z + r.w * r.w <= 1.0f);
    }
    atomicAdd(inside, c);
}
// ANCHOR_END: device_api

int main() {
    float* d;
    CHECK(cudaMalloc(&d, N * sizeof(float)));
    unsigned long long* inside;
    CHECK(cudaMallocManaged(&inside, sizeof(unsigned long long)));

    printf("生成 %zu 个均匀分布的 float：\n", N);
    struct { curandRngType_t type; const char* name; } gens[] = {
        {CURAND_RNG_PSEUDO_XORWOW, "XORWOW（默认）"},
        {CURAND_RNG_PSEUDO_PHILOX4_32_10, "Philox4x32-10"},
        {CURAND_RNG_PSEUDO_MRG32K3A, "MRG32k3a"},
        {CURAND_RNG_PSEUDO_MT19937, "MT19937"},
    };
    for (auto& g : gens) {
        // ANCHOR: host_api
        curandGenerator_t gen;
        CHECK_RAND(curandCreateGenerator(&gen, g.type));
        CHECK_RAND(curandSetPseudoRandomGeneratorSeed(gen, 2026));
        CHECK_RAND(curandGenerateUniform(gen, d, N));     // 预热（含生成器初始化）
        CHECK(cudaDeviceSynchronize());
        double t0 = now_ms();
        CHECK_RAND(curandGenerateUniform(gen, d, N));     // 在显存里生成，(0, 1] 上均匀分布
        CHECK(cudaDeviceSynchronize());
        double ms = now_ms() - t0;
        // ANCHOR_END: host_api
        *inside = 0;
        count_inside<<<26 * 6, 256>>>(d, N / 2, inside);
        CHECK(cudaDeviceSynchronize());
        printf("  %-16s %6.2f ms  %5.0f GB/s   π ≈ %.6f\n", g.name, ms, N * 4.0 / (ms / 1e3) / 1e9,
               4.0 * *inside / (N / 2));
        CHECK_RAND(curandDestroyGenerator(gen));
    }

    // 参照：CPU 单线程用 std::mt19937 生成十分之一的量。
    std::vector<float> h(N / 10);
    std::mt19937 rng(2026);
    std::uniform_real_distribution<float> u(0.0f, 1.0f);
    double t0 = now_ms();
    for (auto& v : h) v = u(rng);
    double cpu = now_ms() - t0;
    printf("  %-16s %6.0f ms（只生成了十分之一，折合全部 %.0f ms）\n", "CPU std::mt19937", cpu, cpu * 10);

    // 设备端 API：每个线程生成 4096 对，比较两种生成器的初始化开销。
    const int blocks = 26 * 6 * 4, threads = 256;
    const size_t per = 4096;
    printf("\n在 kernel 里生成（%d 个线程，每个 %zu 对）：\n", blocks * threads, per);
    const char* names[] = {"XORWOW", "Philox", "Philox，uniform4"};
    for (int which = 0; which < 3; ++which) {
        for (size_t p : {size_t(0), per}) {
            *inside = 0;
            CHECK(cudaDeviceSynchronize());
            double t1 = now_ms();
            if (which == 0) pi_device<curandStateXORWOW_t><<<blocks, threads>>>(p, 2026, inside);
            else if (which == 1) pi_device<curandStatePhilox4_32_10_t><<<blocks, threads>>>(p, 2026, inside);
            else pi_philox4<<<blocks, threads>>>(p, 2026, inside);
            CHECK(cudaGetLastError());
            CHECK(cudaDeviceSynchronize());
            double ms = now_ms() - t1;
            if (p == 0) printf("  %-18s 只做 curand_init：%7.2f ms\n", names[which], ms);
            else printf("  %-18s 初始化并生成：  %7.2f ms   π ≈ %.6f\n", names[which], ms,
                        4.0 * *inside / (double(blocks) * threads * per));
        }
    }
    CHECK(cudaFree(d));
    CHECK(cudaFree(inside));
}
