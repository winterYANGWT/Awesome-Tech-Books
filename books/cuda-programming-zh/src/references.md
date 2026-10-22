# 参考资料与致谢

## 官方文档

书中涉及的 API 行为、硬件参数和平台限制，以 NVIDIA 的官方文档为准。本书的讲解、组织方式和示例都是重新编写的，下面这些文档适合在读完相应章节以后查阅细节：

- [CUDA C++ Programming Guide](https://docs.nvidia.com/cuda/cuda-c-programming-guide/)：编程模型、语言扩展、内存模型和各项功能的权威说明。
- [CUDA C++ Best Practices Guide](https://docs.nvidia.com/cuda/cuda-c-best-practices-guide/)：性能优化的方法和检查清单。
- [CUDA Runtime API](https://docs.nvidia.com/cuda/cuda-runtime-api/) 与 [CUDA Driver API](https://docs.nvidia.com/cuda/cuda-driver-api/)：所有 Runtime 和 Driver 函数的参考手册。
- [NVCC 文档](https://docs.nvidia.com/cuda/cuda-compiler-driver-nvcc/)：编译选项、`-arch` 与 `-gencode`、分离编译（第 4 章）。
- [PTX ISA](https://docs.nvidia.com/cuda/parallel-thread-execution/)：PTX 指令集，第 4、21、35 章涉及。
- [CUDA Math API](https://docs.nvidia.com/cuda/cuda-math-api/)：数学函数的精度上限和快速版本（第 36 章）。
- [Nsight Compute](https://docs.nvidia.com/nsight-compute/) 与 [Nsight Systems](https://docs.nvidia.com/nsight-systems/)：两个性能分析工具的使用手册（第 12、13 章）。
- [CUDA Core Compute Libraries（CCCL）](https://nvidia.github.io/cccl/)：Thrust、CUB 和 libcu++ 的文档（第 24、35 章）。
- [cuBLAS](https://docs.nvidia.com/cuda/cublas/)、[cuSOLVER](https://docs.nvidia.com/cuda/cusolver/)、[cuRAND](https://docs.nvidia.com/cuda/curand/)、[cuFFT](https://docs.nvidia.com/cuda/cufft/)：第五部分用到的各个库。
- [NVRTC](https://docs.nvidia.com/cuda/nvrtc/)：运行时编译库（第 31 章）。

第 33 章用到的 Vulkan 和 EGL 接口，以 Khronos 发布的 [Vulkan 规范](https://registry.khronos.org/vulkan/) 和 [EGL 规范](https://registry.khronos.org/EGL/) 为准。

## 中文图书

- 樊哲勇，《CUDA 编程：基础与实践》，清华大学出版社，2020。第五部分（常用库）和第六部分（分子动力学案例）的示例选题参考了这本书。该书基于 CUDA 10.x 编写，入门部分讲得清楚细致，可以作为本书前几部分的配套读物；书中关于工具（如 nvprof、cuda-memcheck）和具体性能数据的内容已经过时，阅读时需要注意。
