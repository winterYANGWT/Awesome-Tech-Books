// ipc.cu: 两个进程共享同一块显存。父进程分配并填好数据，子进程拿到句柄后打开同一块内存，给每个元素加上 1000000。
// legacy 用 cudaIpcGetMemHandle；vmm 把 cuMemCreate 分配的物理内存导出成文件描述符，经 Unix 域套接字传过去。
//   nvcc -O2 -arch=native ipc.cu -lcuda -o ipc && ./ipc legacy && ./ipc vmm
// NVCCFLAGS: -lcuda
#include <cuda.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <unistd.h>
#include <chrono>
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

const size_t N = 64 << 20;                            // 6400 多万个 int，256 MB
const size_t BYTES = N * sizeof(int);

__global__ void fill(int* a, size_t n) {
    for (size_t i = blockIdx.x * size_t(blockDim.x) + threadIdx.x; i < n; i += size_t(gridDim.x) * blockDim.x)
        a[i] = int(i);
}
__global__ void add(int* a, size_t n, int v) {
    for (size_t i = blockIdx.x * size_t(blockDim.x) + threadIdx.x; i < n; i += size_t(gridDim.x) * blockDim.x)
        a[i] += v;
}

// 普通数据用 write/read 传；文件描述符要用 SCM_RIGHTS 控制消息传，内核会在对方进程里建一个新的描述符。
void send_bytes(int s, const void* p, size_t n) { if (write(s, p, n) != ssize_t(n)) exit(2); }
void recv_bytes(int s, void* p, size_t n) { if (read(s, p, n) != ssize_t(n)) exit(2); }
// ANCHOR: fd
void send_fd(int s, int fd) {
    char dummy = 0, ctrl[CMSG_SPACE(sizeof(int))] = {};
    iovec iov = {&dummy, 1};
    msghdr msg = {};
    msg.msg_iov = &iov, msg.msg_iovlen = 1, msg.msg_control = ctrl, msg.msg_controllen = sizeof ctrl;
    cmsghdr* c = CMSG_FIRSTHDR(&msg);
    c->cmsg_level = SOL_SOCKET, c->cmsg_type = SCM_RIGHTS, c->cmsg_len = CMSG_LEN(sizeof(int));
    memcpy(CMSG_DATA(c), &fd, sizeof fd);
    if (sendmsg(s, &msg, 0) != 1) exit(2);
}
int recv_fd(int s) {
    char dummy, ctrl[CMSG_SPACE(sizeof(int))];
    iovec iov = {&dummy, 1};
    msghdr msg = {};
    msg.msg_iov = &iov, msg.msg_iovlen = 1, msg.msg_control = ctrl, msg.msg_controllen = sizeof ctrl;
    if (recvmsg(s, &msg, 0) != 1) exit(2);
    int fd;
    memcpy(&fd, CMSG_DATA(CMSG_FIRSTHDR(&msg)), sizeof fd);
    return fd;
}
// ANCHOR_END: fd

CUmemAllocationProp prop_fd() {
    CUmemAllocationProp prop = {};
    prop.type = CU_MEM_ALLOCATION_TYPE_PINNED;
    prop.location.type = CU_MEM_LOCATION_TYPE_DEVICE;
    prop.location.id = 0;
    prop.requestedHandleTypes = CU_MEM_HANDLE_TYPE_POSIX_FILE_DESCRIPTOR;   // 创建时就要声明以后要导出
    return prop;
}
CUdeviceptr map_rw(CUmemGenericAllocationHandle h) {
    CUdeviceptr va;
    CU(cuMemAddressReserve(&va, BYTES, 0, 0, 0));
    CU(cuMemMap(va, BYTES, 0, h, 0));
    CUmemAccessDesc acc = {};
    acc.location.type = CU_MEM_LOCATION_TYPE_DEVICE;
    acc.location.id = 0;
    acc.flags = CU_MEM_ACCESS_FLAGS_PROT_READWRITE;
    CU(cuMemSetAccess(va, BYTES, &acc, 1));
    return va;
}

void parent(int s, bool vmm) {
    CHECK(cudaFree(0));
    int* a;
    CUmemGenericAllocationHandle h = 0;
    // ANCHOR: parent
    if (vmm) {
        CUmemAllocationProp prop = prop_fd();
        CU(cuMemCreate(&h, BYTES, &prop, 0));
        a = (int*)map_rw(h);
    } else {
        CHECK(cudaMalloc(&a, BYTES));
    }
    fill<<<1024, 256>>>(a, N);
    cudaEvent_t ready;                                // 跨进程的 event 不能带计时
    CHECK(cudaEventCreateWithFlags(&ready, cudaEventInterprocess | cudaEventDisableTiming));
    CHECK(cudaEventRecord(ready, 0));
    cudaIpcEventHandle_t eh;
    CHECK(cudaIpcGetEventHandle(&eh, ready));
    send_bytes(s, &eh, sizeof eh);                    // 不等 fill 做完就把句柄发出去
    send_bytes(s, &a, sizeof a);                      // 顺便告诉子进程父进程里的地址，只为了对比
    if (vmm) {
        int fd;
        CU(cuMemExportToShareableHandle(&fd, h, CU_MEM_HANDLE_TYPE_POSIX_FILE_DESCRIPTOR, 0));
        send_fd(s, fd);
        close(fd);                                    // 对方收到的是它自己的描述符，这边的可以关掉
    } else {
        cudaIpcMemHandle_t mh;
        CHECK(cudaIpcGetMemHandle(&mh, a));
        send_bytes(s, &mh, sizeof mh);
    }
    // ANCHOR_END: parent
    char done;
    recv_bytes(s, &done, 1);                          // 等子进程做完再检查、释放
    int v[2];
    CHECK(cudaMemcpy(&v[0], a + 7, sizeof(int), cudaMemcpyDeviceToHost));
    CHECK(cudaMemcpy(&v[1], a + N - 1, sizeof(int), cudaMemcpyDeviceToHost));
    printf("父进程：a[7] = %d，a[N-1] = %d\n", v[0], v[1]);
    if (vmm) {
        CU(cuMemUnmap((CUdeviceptr)a, BYTES));
        CU(cuMemAddressFree((CUdeviceptr)a, BYTES));
        CU(cuMemRelease(h));
    } else {
        CHECK(cudaFree(a));
    }
}

void child(int s, bool vmm) {
    int* other;                                       // 子进程先分配一块自己的内存，两边的地址布局就不一样了
    CHECK(cudaMalloc(&other, 64 << 20));
    cudaIpcEventHandle_t eh;
    int* parent_addr;
    recv_bytes(s, &eh, sizeof eh);
    recv_bytes(s, &parent_addr, sizeof parent_addr);
    // ANCHOR: child
    int* a;
    CUmemGenericAllocationHandle h = 0;
    double t0 = now_ms();
    if (vmm) {
        int fd = recv_fd(s);
        t0 = now_ms();
        CU(cuMemImportFromShareableHandle(&h, (void*)(uintptr_t)fd, CU_MEM_HANDLE_TYPE_POSIX_FILE_DESCRIPTOR));
        close(fd);
        a = (int*)map_rw(h);                          // 地址由这个进程自己预留
    } else {
        cudaIpcMemHandle_t mh;
        recv_bytes(s, &mh, sizeof mh);
        t0 = now_ms();
        CHECK(cudaIpcOpenMemHandle((void**)&a, mh, cudaIpcMemLazyEnablePeerAccess));
    }
    double t_open = now_ms() - t0;
    cudaEvent_t ready;
    CHECK(cudaIpcOpenEventHandle(&ready, eh));
    CHECK(cudaStreamWaitEvent(0, ready, 0));          // 在 GPU 上等父进程的 fill 做完
    add<<<1024, 256>>>(a, N, 1000000);
    CHECK(cudaDeviceSynchronize());
    // ANCHOR_END: child
    printf("子进程：打开用了 %.2f ms；父进程里的地址 %p，子进程里的地址 %p\n", t_open, (void*)parent_addr, (void*)a);
    fflush(stdout);
    if (vmm) {
        CU(cuMemUnmap((CUdeviceptr)a, BYTES));
        CU(cuMemAddressFree((CUdeviceptr)a, BYTES));
        CU(cuMemRelease(h));
    } else {
        CHECK(cudaIpcCloseMemHandle(a));
    }
    CHECK(cudaEventDestroy(ready));
    CHECK(cudaFree(other));
    send_bytes(s, "x", 1);
}

int main(int argc, char** argv) {
    bool vmm = argc > 1 && !strcmp(argv[1], "vmm");
    printf("%s：两个进程共享 %zu MB 显存\n", vmm ? "VMM + 文件描述符" : "cudaIpc*", BYTES >> 20);
    fflush(stdout);
    int sv[2];
    if (socketpair(AF_UNIX, SOCK_STREAM, 0, sv)) exit(2);
    pid_t pid = fork();                               // 在任何 CUDA 调用之前 fork
    if (pid == 0) {
        child(sv[1], vmm);
        return 0;
    }
    parent(sv[0], vmm);
    int status;
    waitpid(pid, &status, 0);
    return WEXITSTATUS(status);
}
