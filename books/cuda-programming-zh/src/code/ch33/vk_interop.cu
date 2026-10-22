// vk_interop.cu: Vulkan 分配一块可导出的内存和一个 timeline semaphore，CUDA 导入以后直接在上面计算，用 semaphore 同步。
// 不需要窗口，也不提交 Vulkan 命令：主机通过映射写入数据、给 semaphore 发信号，CUDA 算完再发信号回来。
//   nvcc -O2 -arch=native vk_interop.cu -lvulkan -o vk_interop && ./vk_interop
// NVCCFLAGS: -lvulkan
#include <vulkan/vulkan.h>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <thread>
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
#define VK(call)                                                                       \
    do {                                                                               \
        VkResult r_ = (call);                                                          \
        if (r_ != VK_SUCCESS) {                                                        \
            fprintf(stderr, "%s:%d: VkResult %d\n", __FILE__, __LINE__, int(r_));      \
            exit(1);                                                                   \
        }                                                                              \
    } while (0)

const size_t N = 16 << 20, BYTES = N * sizeof(float);   // 64 MB

__global__ void square(float* a, size_t n) {
    for (size_t i = blockIdx.x * size_t(blockDim.x) + threadIdx.x; i < n; i += size_t(gridDim.x) * blockDim.x)
        a[i] = a[i] * a[i];
}
__global__ void negate(float* a, size_t n) {         // 测带宽用：数值的“样子”不变
    for (size_t i = blockIdx.x * size_t(blockDim.x) + threadIdx.x; i < n; i += size_t(gridDim.x) * blockDim.x)
        a[i] = -a[i];
}
__global__ void fill(float* a, size_t n, bool random) {
    for (size_t i = blockIdx.x * size_t(blockDim.x) + threadIdx.x; i < n; i += size_t(gridDim.x) * blockDim.x) {
        unsigned x = unsigned(i) * 2654435761u;
        x = (x ^ (x >> 13)) * 0x5bd1e995u;
        a[i] = random ? float(x) : 0.0f;
    }
}

float kernel_ms(float* a, bool random, cudaStream_t s) {   // 填好数据，测 negate 的时间
    cudaEvent_t e0, e1;
    CHECK(cudaEventCreate(&e0));
    CHECK(cudaEventCreate(&e1));
    fill<<<208, 256, 0, s>>>(a, N, random);
    negate<<<208, 256, 0, s>>>(a, N);
    CHECK(cudaEventRecord(e0, s));
    for (int r = 0; r < 20; ++r) negate<<<208, 256, 0, s>>>(a, N);
    CHECK(cudaEventRecord(e1, s));
    CHECK(cudaEventSynchronize(e1));
    float ms;
    CHECK(cudaEventElapsedTime(&ms, e0, e1));
    return ms / 20;
}

// ANCHOR: helpers
struct Exported {
    VkBuffer buf;
    VkDeviceMemory mem;
    VkDeviceSize size;
    int fd;
};

// 在 Vulkan 里建一个 buffer，分配带 want 属性、可导出的内存，导出成文件描述符。
Exported export_buffer(VkDevice dev, VkPhysicalDevice pd, VkMemoryPropertyFlags want, PFN_vkGetMemoryFdKHR get_fd) {
    Exported e;
    VkExternalMemoryBufferCreateInfo ebci = {VK_STRUCTURE_TYPE_EXTERNAL_MEMORY_BUFFER_CREATE_INFO};
    ebci.handleTypes = VK_EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUE_FD_BIT;
    VkBufferCreateInfo bci = {VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO, &ebci};
    bci.size = BYTES, bci.usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, bci.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    VK(vkCreateBuffer(dev, &bci, nullptr, &e.buf));
    VkMemoryRequirements req;
    vkGetBufferMemoryRequirements(dev, e.buf, &req);
    VkPhysicalDeviceMemoryProperties mp;
    vkGetPhysicalDeviceMemoryProperties(pd, &mp);
    uint32_t type = 0;
    while (type < mp.memoryTypeCount &&
           (!(req.memoryTypeBits & (1u << type)) || (mp.memoryTypes[type].propertyFlags & want) != want))
        ++type;
    if (type == mp.memoryTypeCount) exit(1);
    VkExportMemoryAllocateInfo emai = {VK_STRUCTURE_TYPE_EXPORT_MEMORY_ALLOCATE_INFO};
    emai.handleTypes = VK_EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUE_FD_BIT;
    VkMemoryAllocateInfo mai = {VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO, &emai};
    mai.allocationSize = req.size, mai.memoryTypeIndex = type;
    VK(vkAllocateMemory(dev, &mai, nullptr, &e.mem));
    VK(vkBindBufferMemory(dev, e.buf, e.mem, 0));
    VkMemoryGetFdInfoKHR mfi = {VK_STRUCTURE_TYPE_MEMORY_GET_FD_INFO_KHR};
    mfi.memory = e.mem, mfi.handleType = VK_EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUE_FD_BIT;
    VK(get_fd(dev, &mfi, &e.fd));
    e.size = req.size;
    VkMemoryPropertyFlags f = mp.memoryTypes[type].propertyFlags;
    printf("导出 %zu MB：内存类型 %u，%s%s\n", size_t(e.size) >> 20, type,
           f & VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT ? "在显存里" : "在主机内存里",
           f & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT ? "，主机可见" : "");
    return e;
}

// CUDA 导入文件描述符，取得整块内存的设备指针。
float* import_buffer(const Exported& e, cudaExternalMemory_t* ext) {
    cudaExternalMemoryHandleDesc emd = {};
    emd.type = cudaExternalMemoryHandleTypeOpaqueFd;
    emd.handle.fd = e.fd;                             // 导入成功后这个 fd 归 CUDA 管，不要再 close
    emd.size = e.size;
    CHECK(cudaImportExternalMemory(ext, &emd));
    cudaExternalMemoryBufferDesc bd = {};
    bd.offset = 0, bd.size = BYTES;
    float* d;
    CHECK(cudaExternalMemoryGetMappedBuffer((void**)&d, *ext, &bd));
    return d;
}
// ANCHOR_END: helpers

int main() {
    // ANCHOR: device
    VkApplicationInfo app = {VK_STRUCTURE_TYPE_APPLICATION_INFO};
    app.apiVersion = VK_API_VERSION_1_2;              // timeline semaphore 从 1.2 起是核心功能
    VkInstanceCreateInfo ici = {VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO};
    ici.pApplicationInfo = &app;
    VkInstance inst;
    VK(vkCreateInstance(&ici, nullptr, &inst));
    // 在 Vulkan 看到的设备里，找 UUID 和 CUDA 设备 0 相同的那个
    cudaDeviceProp cuda_prop;
    CHECK(cudaGetDeviceProperties(&cuda_prop, 0));
    uint32_t count = 0;
    VK(vkEnumeratePhysicalDevices(inst, &count, nullptr));
    std::vector<VkPhysicalDevice> pds(count);
    VK(vkEnumeratePhysicalDevices(inst, &count, pds.data()));
    VkPhysicalDevice pd = VK_NULL_HANDLE;
    for (VkPhysicalDevice p : pds) {
        VkPhysicalDeviceIDProperties id = {VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ID_PROPERTIES};
        VkPhysicalDeviceProperties2 p2 = {VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2, &id};
        vkGetPhysicalDeviceProperties2(p, &p2);
        printf("Vulkan 设备：%s%s\n", p2.properties.deviceName,
               memcmp(id.deviceUUID, cuda_prop.uuid.bytes, VK_UUID_SIZE) ? "" : "（和 CUDA 设备 0 是同一张卡）");
        if (!memcmp(id.deviceUUID, cuda_prop.uuid.bytes, VK_UUID_SIZE)) pd = p;
    }
    if (pd == VK_NULL_HANDLE) return 1;
    VkPhysicalDeviceVulkan12Features f12 = {VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES};
    f12.timelineSemaphore = VK_TRUE;
    const char* exts[] = {VK_KHR_EXTERNAL_MEMORY_FD_EXTENSION_NAME, VK_KHR_EXTERNAL_SEMAPHORE_FD_EXTENSION_NAME};
    float prio = 1;
    VkDeviceQueueCreateInfo qci = {VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO};
    qci.queueFamilyIndex = 0, qci.queueCount = 1, qci.pQueuePriorities = &prio;
    VkDeviceCreateInfo dci = {VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO, &f12};
    dci.queueCreateInfoCount = 1, dci.pQueueCreateInfos = &qci;
    dci.enabledExtensionCount = 2, dci.ppEnabledExtensionNames = exts;
    VkDevice dev;
    VK(vkCreateDevice(pd, &dci, nullptr, &dev));
    auto vkGetMemoryFd = (PFN_vkGetMemoryFdKHR)vkGetDeviceProcAddr(dev, "vkGetMemoryFdKHR");
    auto vkGetSemaphoreFd = (PFN_vkGetSemaphoreFdKHR)vkGetDeviceProcAddr(dev, "vkGetSemaphoreFdKHR");
    // ANCHOR_END: device

    // ANCHOR: export
    // 两块可导出的内存：一块主机可见（主机能直接映射读写），一块在显存里。
    Exported host_buf = export_buffer(dev, pd, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                                      vkGetMemoryFd);
    Exported vram_buf = export_buffer(dev, pd, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, vkGetMemoryFd);
    // 一个可导出的 timeline semaphore，初值 0
    VkSemaphoreTypeCreateInfo stci = {VK_STRUCTURE_TYPE_SEMAPHORE_TYPE_CREATE_INFO};
    stci.semaphoreType = VK_SEMAPHORE_TYPE_TIMELINE, stci.initialValue = 0;
    VkExportSemaphoreCreateInfo esci = {VK_STRUCTURE_TYPE_EXPORT_SEMAPHORE_CREATE_INFO, &stci};
    esci.handleTypes = VK_EXTERNAL_SEMAPHORE_HANDLE_TYPE_OPAQUE_FD_BIT;
    VkSemaphoreCreateInfo sci = {VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO, &esci};
    VkSemaphore sem;
    VK(vkCreateSemaphore(dev, &sci, nullptr, &sem));
    VkSemaphoreGetFdInfoKHR sfi = {VK_STRUCTURE_TYPE_SEMAPHORE_GET_FD_INFO_KHR};
    sfi.semaphore = sem, sfi.handleType = VK_EXTERNAL_SEMAPHORE_HANDLE_TYPE_OPAQUE_FD_BIT;
    int sem_fd;
    VK(vkGetSemaphoreFd(dev, &sfi, &sem_fd));
    // ANCHOR_END: export

    // ANCHOR: import
    cudaExternalMemory_t host_ext, vram_ext;
    float* d = import_buffer(host_buf, &host_ext);
    float* d_vram = import_buffer(vram_buf, &vram_ext);
    cudaExternalSemaphoreHandleDesc esd = {};
    esd.type = cudaExternalSemaphoreHandleTypeTimelineSemaphoreFd;
    esd.handle.fd = sem_fd;
    cudaExternalSemaphore_t ext_sem;
    CHECK(cudaImportExternalSemaphore(&ext_sem, &esd));
    // ANCHOR_END: import

    // ANCHOR: sync
    float* h;
    VK(vkMapMemory(dev, host_buf.mem, 0, BYTES, 0, (void**)&h));
    for (size_t i = 0; i < N; ++i) h[i] = float(i % 1000);   // “Vulkan 这边”准备数据
    cudaStream_t s;
    CHECK(cudaStreamCreate(&s));
    // 先空跑一次 kernel。默认的按需加载（第 34 章）下，kernel 第一次启动时才装载模块；
    // 如果这时 stream 正在等一个要到启动返回以后才会发出的信号，启动这一行就会卡住不返回。
    square<<<1, 32, 0, s>>>(d, 0);
    cudaExternalSemaphoreWaitParams wp = {};
    wp.params.fence.value = 1;                        // 等 semaphore 到 1
    CHECK(cudaWaitExternalSemaphoresAsync(&ext_sem, &wp, 1, s));
    square<<<208, 256, 0, s>>>(d, N);
    cudaExternalSemaphoreSignalParams sp = {};
    sp.params.fence.value = 2;                        // 算完把它设成 2
    CHECK(cudaSignalExternalSemaphoresAsync(&ext_sem, &sp, 1, s));
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    printf("任务已经排进 stream，100 ms 后 stream 的状态：%s\n", cudaGetErrorName(cudaStreamQuery(s)));
    VkSemaphoreSignalInfo ssi = {VK_STRUCTURE_TYPE_SEMAPHORE_SIGNAL_INFO};
    ssi.semaphore = sem, ssi.value = 1;
    VK(vkSignalSemaphore(dev, &ssi));                 // 主机发信号：数据准备好了
    uint64_t want_value = 2;
    VkSemaphoreWaitInfo swi = {VK_STRUCTURE_TYPE_SEMAPHORE_WAIT_INFO};
    swi.semaphoreCount = 1, swi.pSemaphores = &sem, swi.pValues = &want_value;
    VK(vkWaitSemaphores(dev, &swi, UINT64_MAX));      // 在 Vulkan 这边等 CUDA 算完
    size_t wrong = 0;
    for (size_t i = 0; i < N; i += 4097) wrong += h[i] != float(i % 1000) * float(i % 1000);
    printf("Vulkan 发信号以后 CUDA 才开始算；在 Vulkan 这边检查结果，错误 %zu 个\n", wrong);
    // ANCHOR_END: sync

    float* plain;
    CHECK(cudaMalloc(&plain, BYTES));
    printf("\n读写一遍 64 MB    导入的主机内存   导入的显存   cudaMalloc 的显存\n");
    for (bool random : {true, false})
        printf("%s           %8.3f ms     %8.3f ms     %8.3f ms\n", random ? "随机数据" : "全零    ",
               kernel_ms(d, random, s), kernel_ms(d_vram, random, s), kernel_ms(plain, random, s));

    CHECK(cudaFree(plain));
    CHECK(cudaStreamDestroy(s));
    CHECK(cudaDestroyExternalSemaphore(ext_sem));
    for (float* p : {d, d_vram}) CHECK(cudaFree(p));  // 映射出来的指针也要 cudaFree
    CHECK(cudaDestroyExternalMemory(host_ext));
    CHECK(cudaDestroyExternalMemory(vram_ext));
    vkUnmapMemory(dev, host_buf.mem);
    vkDestroySemaphore(dev, sem, nullptr);
    for (Exported* e : {&host_buf, &vram_buf}) {
        vkDestroyBuffer(dev, e->buf, nullptr);
        vkFreeMemory(dev, e->mem, nullptr);
    }
    vkDestroyDevice(dev, nullptr);
    vkDestroyInstance(inst, nullptr);
}
