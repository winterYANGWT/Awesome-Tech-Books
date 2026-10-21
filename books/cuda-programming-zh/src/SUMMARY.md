# Summary

[前言](index.md)

# 第一部分 CUDA 世界观与平台模型

- [第 1 章 CUDA 为什么存在：GPU 不是更快的 CPU](ch001.md)
- [第 2 章 一个 CUDA 程序到底跑在哪里：host、device 与异构生命周期](ch002.md)
- [第 3 章 从 SM 看 GPU：线程不是独立跑在一块抽象设备上](ch003.md)
- [第 4 章 PTX、cubin、fatbin 与 JIT：CUDA 程序为什么有时第一次运行更慢](ch004.md)
- [第 5 章 先有地图再查 API：CUDA Feature Tour 应该怎么读](ch005.md)

# 第二部分 CUDA C++ kernel 核心

- [第 6 章 第一个完整 CUDA C++ 程序：不只是一段 kernel](ch006.md)
- [第 7 章 Thread、Block、Grid：CUDA 坐标系统怎么把数据分给线程](ch007.md)
- [第 8 章 Kernel launch 不是函数调用：异步错误为什么会晚一点出现](ch008.md)
- [第 9 章 SIMT 与 warp：每个线程写标量代码，硬件按组执行](ch009.md)
- [第 10 章 kernel 视角下的内存空间：global、shared、local、constant、texture、surface](ch010.md)
- [第 11 章 从能写 kernel 到能表达约束：`__restrict__`、cluster 与动态 shared memory](ch011.md)

# 第三部分 Stream、Event 与 nvcc

- [第 12 章 Stream 是 GPU 工作队列：异步不是自动并行](ch012.md)
- [第 13 章 Event 不只是计时：它是 stream 之间的依赖标记](ch013.md)
- [第 14 章 隐式同步和异步错误：CUDA 程序最容易误判的两条边界](ch014.md)
- [第 15 章 把 host 代码和复杂拷贝放进异步流程](ch015.md)
- [第 16 章 nvcc 为什么存在：host code 和 device code 是两条编译链](ch016.md)
- [第 17 章 `-arch`、`-gencode`、RDC 与调试构建：让二进制跑在正确的 GPU 上](ch017.md)
- [第 18 章 异步回调与批量拷贝 reference：把少见但重要的 stream API 补齐](ch018.md)

# 第四部分 数据放置与 Unified Memory

- [第 19 章 UVA、Unified Memory、Managed Memory：先把名字分清楚](ch019.md)
- [第 20 章 Managed Memory 的性能核心：page fault、迁移、prefetch 与 advice](ch020.md)
- [第 21 章 Full 与 Limited Unified Memory：平台差异决定写法保守程度](ch021.md)
- [第 22 章 Pinned 与 Mapped Host Memory：异步拷贝和零拷贝的前提](ch022.md)
- [第 23 章 多 GPU 下的 managed memory：可见性、迁移和一致性边界](ch023.md)
- [第 24 章 数据放置排错：慢、不一致、偶发 page fault 时怎么查](ch024.md)
- [第 25 章 Unified Memory reference：advice、prefetch、range attribute 与 stream attach](ch025.md)

# 第五部分 Kernel 性能边界

- [第 26 章 SIMT 的性能含义：warp 是执行单位，线程是编程单位](ch026.md)
- [第 27 章 Occupancy 不是越高越好：register、shared memory、local memory 如何约束并发](ch027.md)
- [第 28 章 Memory access 才是很多 kernel 的上限：coalescing、bank conflict 与 cache](ch028.md)
- [第 29 章 Compute Capability 不是背表：它告诉你哪些优化真的存在](ch029.md)
- [第 30 章 一个实用的 kernel 性能排查顺序](ch030.md)

# 第六部分 Runtime、Driver 与多 GPU

- [第 31 章 三尖括号不够用时：Runtime 高级 launch 与属性化 kernel 提交](ch031.md)
- [第 32 章 Driver API 最小路径：device、context、module、function、launch](ch032.md)
- [第 33 章 Runtime API 和 Driver API 能混用，但你必须知道上下文是谁的](ch033.md)
- [第 34 章 多 GPU 程序的第一步：设备选择、peer access 与拓扑判断](ch034.md)
- [第 35 章 CUDA Feature Tour 到工程决策：不要把所有高级 API 都当默认选择](ch035.md)
- [第 36 章 Driver JIT/linker reference：从 PTX 到 module 的运行时装配路径](ch036.md)

# 第七部分 CUDA Graph 与 Stream-Ordered Allocator

- [第 37 章 CUDA Graph 为什么存在：降低重复提交开销，而不是让代码自动变快](ch037.md)
- [第 38 章 Stream Capture 的规则：你捕获的是依赖，不是随便录屏](ch038.md)
- [第 39 章 Graph Update、Memory Node 与 Device Graph Launch：Graph 不是一次性对象](ch039.md)
- [第 40 章 Graph 中的资源生命周期：User Object 解决谁来释放的问题](ch040.md)
- [第 41 章 `cudaMallocAsync`：把 malloc/free 放进 stream 顺序里](ch041.md)
- [第 42 章 CUDA Graph 速查：节点类型、更新与条件执行](ch042.md)

# 第八部分 现代 kernel 协作机制

- [第 43 章 Cooperative Groups：把“谁在一起协作”写进类型和对象里](ch043.md)
- [第 44 章 从 block 到 cluster：更大范围协作的代价和边界](ch044.md)
- [第 45 章 Programmatic Dependent Launch：让 kernel 之间的依赖更早暴露](ch045.md)
- [第 46 章 Asynchronous Barrier：把 arrive 和 wait 拆开以后，程序结构发生什么变化](ch046.md)
- [第 47 章 Pipeline：把加载和计算做成真正的生产者-消费者](ch047.md)
- [第 48 章 Kernel 内部异步拷贝：LDGSTS 与 `cuda::memcpy_async`](ch048.md)
- [第 49 章 TMA：Tensor Memory Accelerator 的 host/device 双侧流程](ch049.md)
- [第 50 章 L2 Cache Control：哪些数据值得被“持久化”留在 L2](ch050.md)
- [第 51 章 Memory Synchronization Domains：减少不必要的 fence 干扰](ch051.md)
- [第 52 章 TMA、L2 持久化与内存同步域：现代 Kernel 协作的细节索引](ch052.md)

# 第九部分 运行时资源与可观测性

- [第 53 章 Green Context：把 GPU 资源切给不同工作负载](ch053.md)
- [第 54 章 Lazy Loading：第一次慢可能是模块加载，不是 kernel 慢](ch054.md)
- [第 55 章 Error Log Management：让 CUDA 错误有可读上下文](ch055.md)
- [第 56 章 CUDA 运行时可观测性手册：从设备可见性到 JIT、加载和错误](ch056.md)

# 第十部分 系统级内存

- [第 57 章 Legacy CUDA IPC：跨进程共享 device memory 的简单路径和限制](ch057.md)
- [第 58 章 VMM：把虚拟地址、物理内存和访问权限拆开管理](ch058.md)
- [第 59 章 用 VMM 做跨进程共享：POSIX fd、Win32 handle 与安全边界](ch059.md)
- [第 60 章 Multicast Objects：为什么系统库需要比 peer access 更底层的能力](ch060.md)
- [第 61 章 EGM 与 Compressible Memory：当 GPU 内存边界延伸到系统级内存池](ch061.md)
- [第 62 章 VMM、平台句柄与跨进程内存：从虚拟地址到可共享资源](ch062.md)

# 第十一部分 CDP、Interop 与 Driver Entry Point

- [第 63 章 CUDA Dynamic Parallelism：GPU 端 launch kernel 解决什么问题](ch063.md)
- [第 64 章 CDP 的内存一致性和低层 API：global 可以共享，shared/local 不行](ch064.md)
- [第 65 章 OpenGL/D3D 互操作：register、map、use、unmap 的资源所有权](ch065.md)
- [第 66 章 Vulkan/D3D12 互操作：external memory 与 external semaphore 是同一套边界问题](ch066.md)
- [第 67 章 NVSCI 互操作：安全系统里的共享内存和同步对象](ch067.md)
- [第 68 章 `cuGetProcAddress`：动态获取 Driver API 函数指针时，版本和 ABI 才是重点](ch068.md)
- [第 69 章 CUDA 互操作参考表：D3D、Vulkan、NvSci 与外部同步对象](ch069.md)

# 第十二部分 语言扩展、数值行为与参考手册

- [第 70 章 CUDA 环境变量：不改代码也能改变设备、JIT、加载和错误观察方式](ch070.md)
- [第 71 章 CUDA C++ 不是普通 C++ 加三尖括号：标准支持、lambda 和限制条件](ch071.md)
- [第 72 章 CUDA 语言扩展：执行空间、存储空间和优化修饰符不是装饰](ch072.md)
- [第 73 章 同步、Fence、Atomic 和 Warp Functions：先问清楚作用域](ch073.md)
- [第 74 章 GPU 和 CPU 浮点结果不同，不自动说明谁错了](ch074.md)
- [第 75 章 专用指令和 Device-callable API：DPX、WMMA、device runtime 不是日常语法](ch075.md)
- [第 76 章 CUDA C++ Memory Model 与 Execution Model：把 C++ 并发语义放到 GPU 上会变形](ch076.md)
- [第 77 章 CUDA 语言扩展与数值函数完整索引：把参考手册放回工程语境](ch077.md)

# 第十三部分 CUDA 常用库实践

- [第 78 章 Thrust prefix scan：从容器接口到 raw device pointer](ch078.md)
- [第 79 章 cuBLAS GEMM：column-major、leading dimension 和 handle 生命周期](ch079.md)
- [第 80 章 cuSolver eigenvalue：workspace、devInfo 与状态检查](ch080.md)
- [第 81 章 cuRAND Host API：随机数生成器、seed 与 device array](ch081.md)
- [第 82 章 cuFFT plan 边界 checklist](ch082.md)
- [第 83 章 CUDA 库错误处理 reference：不要只检查 Runtime API](ch083.md)

# 第十四部分 端到端案例

- [第 84 章 MD C++ baseline：从邻居表到热点定位](ch084.md)
- [第 85 章 MD force-only CUDA port：一个线程负责一个粒子](ch085.md)
- [第 86 章 MD whole-code data residency：为什么只迁移一个 kernel 不够](ch086.md)
- [第 87 章 MD 数据布局与时间步数据流：从数组位置判断迁移边界](ch087.md)

---

[参考资料与致谢](references.md)
[版权与许可](license.md)
