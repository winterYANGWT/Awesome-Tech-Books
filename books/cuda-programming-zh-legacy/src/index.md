# 前言

> 这是本书的旧版，已不再更新。旧版按主题逐项讲解，共 14 部分 87 章，每章附 API 速查，适合按主题查阅。新版重写为 7 部分 36 章，每章围绕能编译运行的程序和实测数据展开，建议阅读[新版](https://winteryangwt.github.io/Awesome-Tech-Books/cuda-programming-zh/)。

这是一本以工程问题为线索的 CUDA 中文教程。它覆盖的主题大致对应 NVIDIA 官方《CUDA C++ Programming Guide》：执行模型、kernel 编程、stream 与 event、数据放置、性能、CUDA Graph、多 GPU、kernel 内的协作机制、系统级内存、互操作和语言参考。不过本书的组织方式、讲解和示例都是重新写的，不是官方文档的翻译。

官方指南是一份按功能排列的参考资料，适合查阅；本书想解决的是另一个问题：一个已经会写 C++ 的工程师，怎样建立起足够的 CUDA 心智模型，在写代码、读性能数据、排查问题时能做出正确判断。所以每章都从一个实际问题出发，先讲清楚模型，再给代码和取舍。

## 适合谁读

- 熟悉 C++，想系统学习 CUDA 的开发者；
- 已经写过一些 kernel，但对异步、同步、内存放置和性能经常“凭感觉”的工程师；
- 需要了解 CUDA Graph、cluster、TMA、VMM、Green Context 等较新机制的读者。

读者需要掌握 C++ 基础。书中不会从零讲 C++，也不会讲 GPU 的电路细节。

## 全书结构

| 部分 | 章 | 内容 |
|---|---|---|
| [第一部分 CUDA 世界观与平台模型](ch001.md) | 1–5 | GPU 和 CPU 的设计取向、host/device 分工、SM 执行模型、编译产物 |
| [第二部分 CUDA C++ kernel 核心](ch006.md) | 6–11 | 第一个完整程序、线程坐标、launch 语义、SIMT 与 warp、内存空间 |
| [第三部分 Stream、Event 与 nvcc](ch012.md) | 12–18 | 异步执行、依赖与同步、错误暴露、nvcc 编译链 |
| [第四部分 数据放置与 Unified Memory](ch019.md) | 19–25 | UVA、managed memory、pinned/mapped memory、多 GPU 下的数据位置 |
| [第五部分 Kernel 性能边界](ch026.md) | 26–30 | warp 执行效率、occupancy、访存模式、compute capability、排查顺序 |
| [第六部分 Runtime、Driver 与多 GPU](ch031.md) | 31–36 | 高级 launch、Driver API、上下文、多 GPU 拓扑 |
| [第七部分 CUDA Graph 与 Stream-Ordered Allocator](ch037.md) | 37–42 | graph 模型、stream capture、更新与资源生命周期、`cudaMallocAsync` |
| [第八部分 现代 kernel 协作机制](ch043.md) | 43–52 | cooperative groups、cluster、PDL、异步 barrier、pipeline、TMA、L2 控制 |
| [第九部分 运行时资源与可观测性](ch053.md) | 53–56 | Green Context、lazy loading、错误日志、环境变量与排查 |
| [第十部分 系统级内存](ch057.md) | 57–62 | CUDA IPC、VMM、跨进程共享、multicast、EGM |
| [第十一部分 CDP、Interop 与 Driver Entry Point](ch063.md) | 63–69 | 动态并行、图形 API 互操作、外部内存与信号量、`cuGetProcAddress` |
| [第十二部分 语言扩展、数值行为与参考手册](ch070.md) | 70–77 | 语言扩展、同步与原子操作、浮点行为、内存模型 |
| [第十三部分 CUDA 常用库实践](ch078.md) | 78–83 | Thrust、cuBLAS、cuSolver、cuRAND、cuFFT 与库的错误处理 |
| [第十四部分 端到端案例](ch084.md) | 84–87 | 把一个分子动力学程序逐步迁移到 GPU |

## 怎么读

- **入门**：按顺序读第一到第三部分，再读第四部分前两章。读完就能写出结构正确的 CUDA 程序，并且知道异步和同步的边界在哪里。
- **做性能**：入门之后读第五部分，再按需要读第七、八部分。
- **做系统集成**：第六、十、十一部分彼此相对独立，可以按项目需要挑着读。
- **练手**：第十三、十四部分是库调用和完整案例，读完前三部分就可以穿插阅读。
- 每部分末尾通常有一章偏参考性质，汇总这一部分涉及的 API 和细节，适合查阅，不必通读。

每章的结构基本相同：

- **问题的由来**：这个主题在真实程序里为什么会出现；
- **核心模型**：先建立一张关系图或一个心智模型；
- **详解**：按理解顺序展开；
- **代码示例**：最小可用的调用顺序；
- **工程判断**：什么时候用、什么时候不用、容易在哪里误判；
- **API 与要点速查**：本章涉及的 API、参数和限制；
- **小结**：读完本章应该具备的判断能力。

## 约定

- stream、warp、kernel、thread block、shared memory 等术语保留英文，第一次出现时会解释含义。
- 书中代码大多是为说明问题而截取的片段，通常省略了错误检查和资源释放。真实项目中每个 CUDA API 的返回值都应该检查，第 6 章和第 14 章专门讨论这一点。
- 示例代码按 CUDA 13 的 API 编写，和 CUDA 12.x 写法不同的地方会注明。较新的特性往往依赖特定的 GPU 架构、驱动版本或操作系统，书中会尽量标明；具体 API 签名和支持矩阵请以你所用版本的官方文档为准。

## 声明

本书不是 NVIDIA 的官方文档，也没有经过 NVIDIA 审阅或认可。NVIDIA 和 CUDA 是 NVIDIA Corporation 在美国和其他国家的商标或注册商标。

本书在 AI 辅助下编写和整理，难免有错误或过时之处。发现问题欢迎到 [GitHub 仓库](https://github.com/winterYANGWT/Awesome-Tech-Books) 提 issue。

本书文字采用 [CC BY 4.0](https://creativecommons.org/licenses/by/4.0/deed.zh-hans) 许可，示例代码采用 MIT 许可，详见[版权与许可](license.md)。
