// message.cu: 两个常驻的 block 在一个 kernel 里来回传 2000 轮消息。
// 生产者写好数据再置标志，消费者等到标志再读数据，读完回一个确认，生产者等到确认再写下一轮。
// 比较三种写法：普通变量、volatile、cuda::atomic_ref 的 release/acquire。
//   nvcc -O2 -arch=native message.cu -o message && ./message
#include <cuda/atomic>
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

constexpr int WORDS = 4096;                  // 一条消息 4096 个 int
constexpr int ROUNDS = 2000;
constexpr long long SPIN_LIMIT = 10'000'000; // 每次最多等这么多次，免得卡死

struct Shared {
    int data[WORDS];
    int flag;                                // 生产者：第几轮的数据已经写好
    int ack;                                 // 消费者：第几轮的数据已经读完
    int stale;                               // 消费者读到的旧数据个数
    int timeouts;                            // 等不到对方的次数
};

// ANCHOR: plain
// 写法一：全是普通变量。
__global__ void exchange_plain(Shared* s) {
    for (int r = 1; r <= ROUNDS; ++r) {
        if (blockIdx.x == 0) {               // 生产者
            if (threadIdx.x == 0) {
                long long spins = 0;
                while (s->ack != r - 1 && ++spins < SPIN_LIMIT) {}
                if (spins == SPIN_LIMIT) ++s->timeouts;
            }
            __syncthreads();
            for (int i = threadIdx.x; i < WORDS; i += blockDim.x) s->data[i] = r;
            __syncthreads();
            if (threadIdx.x == 0) s->flag = r;
        } else {                             // 消费者
            if (threadIdx.x == 0) {
                long long spins = 0;
                while (s->flag != r && ++spins < SPIN_LIMIT) {}
                if (spins == SPIN_LIMIT) ++s->timeouts;
            }
            __syncthreads();
            int stale = 0;
            for (int i = threadIdx.x; i < WORDS; i += blockDim.x) stale += s->data[i] != r;
            if (stale) atomicAdd(&s->stale, stale);
            __syncthreads();
            if (threadIdx.x == 0) s->ack = r;
        }
    }
}
// ANCHOR_END: plain

// 写法二：标志和确认用 volatile，数据仍是普通变量。
__global__ void exchange_volatile(Shared* s) {
    volatile int* flag = &s->flag;
    volatile int* ack = &s->ack;
    for (int r = 1; r <= ROUNDS; ++r) {
        if (blockIdx.x == 0) {
            if (threadIdx.x == 0) {
                long long spins = 0;
                while (*ack != r - 1 && ++spins < SPIN_LIMIT) {}
                if (spins == SPIN_LIMIT) ++s->timeouts;
            }
            __syncthreads();
            for (int i = threadIdx.x; i < WORDS; i += blockDim.x) s->data[i] = r;
            __syncthreads();
            if (threadIdx.x == 0) *flag = r;
        } else {
            if (threadIdx.x == 0) {
                long long spins = 0;
                while (*flag != r && ++spins < SPIN_LIMIT) {}
                if (spins == SPIN_LIMIT) ++s->timeouts;
            }
            __syncthreads();
            int stale = 0;
            for (int i = threadIdx.x; i < WORDS; i += blockDim.x) stale += s->data[i] != r;
            if (stale) atomicAdd(&s->stale, stale);
            __syncthreads();
            if (threadIdx.x == 0) *ack = r;
        }
    }
}

// ANCHOR: acquire_release
// 写法三：标志和确认用原子操作。写的一方 release，读的一方 acquire。
__global__ void exchange_atomic(Shared* s) {
    cuda::atomic_ref<int, cuda::thread_scope_device> flag(s->flag), ack(s->ack);
    for (int r = 1; r <= ROUNDS; ++r) {
        if (blockIdx.x == 0) {
            if (threadIdx.x == 0) {
                long long spins = 0;
                while (ack.load(cuda::memory_order_acquire) != r - 1 && ++spins < SPIN_LIMIT) {}
                if (spins == SPIN_LIMIT) ++s->timeouts;
            }
            __syncthreads();
            for (int i = threadIdx.x; i < WORDS; i += blockDim.x) s->data[i] = r;
            __syncthreads();
            if (threadIdx.x == 0) flag.store(r, cuda::memory_order_release);   // 之前的写对 acquire 方可见
        } else {
            if (threadIdx.x == 0) {
                long long spins = 0;
                while (flag.load(cuda::memory_order_acquire) != r && ++spins < SPIN_LIMIT) {}
                if (spins == SPIN_LIMIT) ++s->timeouts;
            }
            __syncthreads();
            int stale = 0;
            for (int i = threadIdx.x; i < WORDS; i += blockDim.x) stale += s->data[i] != r;
            if (stale) atomicAdd(&s->stale, stale);
            __syncthreads();
            if (threadIdx.x == 0) ack.store(r, cuda::memory_order_release);
        }
    }
}
// ANCHOR_END: acquire_release

int main() {
    Shared* s;
    CHECK(cudaMallocManaged(&s, sizeof(Shared)));
    auto run = [&](const char* label, auto kernel) {
        CHECK(cudaMemset(s, 0, sizeof(Shared)));
        CHECK(cudaDeviceSynchronize());
        kernel<<<2, 256>>>(s);                // block 0 生产，block 1 消费，两个 block 同时驻留
        CHECK(cudaGetLastError());
        CHECK(cudaDeviceSynchronize());
        printf("%-28s 读到旧数据 %7d 个，等不到对方 %4d 次\n", label, s->stale, s->timeouts);
    };
    run("普通变量", exchange_plain);
    run("volatile 标志", exchange_volatile);
    run("atomic_ref release/acquire", exchange_atomic);
    CHECK(cudaFree(s));
}
