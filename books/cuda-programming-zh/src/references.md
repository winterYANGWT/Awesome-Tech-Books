# 参考资料与致谢

## 官方文档

本书的主题范围以 NVIDIA 官方文档为主线。书中的讲解、组织方式和示例都是重新编写的；遇到具体 API 签名、硬件支持矩阵和平台限制，请以官方文档为准。

- [CUDA C++ Programming Guide](https://docs.nvidia.com/cuda/cuda-c-programming-guide/)：CUDA 编程模型和语言扩展的权威说明，本书大部分主题的出处。
- [CUDA C++ Best Practices Guide](https://docs.nvidia.com/cuda/cuda-c-best-practices-guide/)：性能优化的方法和检查清单。
- [CUDA Runtime API](https://docs.nvidia.com/cuda/cuda-runtime-api/) 与 [CUDA Driver API](https://docs.nvidia.com/cuda/cuda-driver-api/)：所有 runtime/driver 函数的参考手册。
- [NVCC 文档](https://docs.nvidia.com/cuda/cuda-compiler-driver-nvcc/)：编译选项、`-arch`/`-gencode` 与分离编译。
- [CUDA 数学库与常用库文档](https://docs.nvidia.com/cuda/)：cuBLAS、cuSOLVER、cuRAND、cuFFT、Thrust 等库的参考手册，对应第十三部分。

## 中文图书

- 樊哲勇，《CUDA 编程：基础与实践》，清华大学出版社，2020。第十三部分（常用库实践）和第十四部分（分子动力学案例）的示例选题参考了这本书。该书基于 CUDA 10.x 编写，入门部分讲得清楚细致，适合作为本书前几部分的配套读物；书中关于工具（如 nvprof、cuda-memcheck）和具体性能数据的内容已经过时，阅读时需要注意。
