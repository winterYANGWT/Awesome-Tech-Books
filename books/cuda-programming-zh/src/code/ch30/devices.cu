// devices.cu: 列出进程能看到的 GPU，以及每两张卡之间能不能直接访问对方的显存。
//   nvcc -O2 devices.cu -o devices && ./devices
//   CUDA_VISIBLE_DEVICES= ./devices      # 一张卡也看不到
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

int main() {
    const char* visible = getenv("CUDA_VISIBLE_DEVICES");
    printf("CUDA_VISIBLE_DEVICES=%s\n", visible ? visible : "（未设置）");
    fflush(stdout);                                   // 出错信息走 stderr，先把这一行输出
    // ANCHOR: list
    int count = 0;
    CHECK(cudaGetDeviceCount(&count));
    for (int d = 0; d < count; ++d) {
        cudaDeviceProp prop;
        CHECK(cudaGetDeviceProperties(&prop, d));
        char bus[32];
        CHECK(cudaDeviceGetPCIBusId(bus, sizeof bus, d));
        printf("设备 %d：%s，计算能力 %d.%d，%zu MB，PCI %s，UUID GPU-", d, prop.name, prop.major,
               prop.minor, prop.totalGlobalMem >> 20, bus);
        for (int k = 0; k < 16; ++k)
            printf("%02x%s", (unsigned char)prop.uuid.bytes[k], k == 3 || k == 5 || k == 7 || k == 9 ? "-" : "");
        printf("\n");
    }
    // ANCHOR_END: list
    // ANCHOR: peer
    printf("\n能否直接访问对方显存（行访问列）：\n");
    for (int a = 0; a < count; ++a) {
        for (int b = 0; b < count; ++b) {
            if (a == b) { printf("   -"); continue; }
            int can = 0, rank = 0;
            CHECK(cudaDeviceCanAccessPeer(&can, a, b));
            if (can) CHECK(cudaDeviceGetP2PAttribute(&rank, cudaDevP2PAttrPerformanceRank, a, b));
            printf(can ? "  P%d" : "   x", rank);
        }
        printf("\n");
    }
    // ANCHOR_END: peer
}
