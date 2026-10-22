# Summary

[前言](index.md)

# 第一部分 起步

- [第 1 章 GPU 擅长什么：从一个 for 循环说起](ch01.md)
- [第 2 章 第一个完整程序：分配、拷贝、启动、检查](ch02.md)
- [第 3 章 线程怎么分到数据上](ch03.md)
- [第 4 章 nvcc 做了什么](ch04.md)
- [第 5 章 出错了怎么办](ch05.md)

# 第二部分 写出快的 kernel

- [第 6 章 warp：硬件真正的执行单位](ch06.md)
- [第 7 章 内存空间一览](ch07.md)
- [第 8 章 合并访问](ch08.md)
- [第 9 章 shared memory：矩阵转置与 bank conflict](ch09.md)
- [第 10 章 occupancy：驻留的 warp 越多越好吗](ch10.md)
- [第 11 章 归约：从最直接的写法到跑满带宽](ch11.md)
- [第 12 章 用 Nsight Compute 找瓶颈](ch12.md)

# 第三部分 让 CPU、拷贝和 GPU 同时忙起来

- [第 13 章 Stream：让拷贝和计算重叠起来](ch13.md)
- [第 14 章 Event 与跨 stream 依赖](ch14.md)
- [第 15 章 主机内存：pageable、pinned 与零拷贝](ch15.md)
- [第 16 章 Unified Memory：按需迁移的代价](ch16.md)
- [第 17 章 CUDA Graph：把重复提交的开销省掉](ch17.md)
- [第 18 章 流序内存分配：cudaMallocAsync 与内存池](ch18.md)

# 第四部分 现代 GPU 上的协作机制

- [第 19 章 Cooperative Groups 与 thread block cluster](ch19.md)
- [第 20 章 异步拷贝、barrier 与 pipeline](ch20.md)
- [第 21 章 TMA：用描述符搬运多维数据块](ch21.md)
- [第 22 章 内存模型：atomic、fence 与作用域](ch22.md)
- [第 23 章 L2 持久化与 Programmatic Dependent Launch](ch23.md)

# 第五部分 用好现成的库

- [第 24 章 Thrust 与 CUB](ch24.md)
- [第 25 章 cuBLAS 与 cuSOLVER](ch25.md)
- [第 26 章 cuRAND 与 cuFFT](ch26.md)

# 第六部分 案例：把一个分子动力学程序搬上 GPU

- [第 27 章 CPU 基线与热点](ch27.md)
- [第 28 章 先移植求力](ch28.md)
- [第 29 章 让数据留在 GPU 上](ch29.md)

# 第七部分 系统与平台

- [第 30 章 多 GPU](ch30.md)
- [第 31 章 Driver API](ch31.md)
- [第 32 章 跨进程共享与虚拟内存管理](ch32.md)
- [第 33 章 与图形和外部 API 互操作](ch33.md)
- [第 34 章 调节运行时行为](ch34.md)
- [第 35 章 CUDA C++ 能用到什么程度](ch35.md)
- [第 36 章 GPU 和 CPU 的浮点结果为什么不一样](ch36.md)

---

[参考资料与致谢](references.md)
[版权与许可](license.md)
