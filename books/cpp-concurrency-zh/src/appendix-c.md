# 附录 C 术语对照

本书正文第一次出现术语时给英文，之后只用中文或接口名。下表按主题分组；“其他资料常见译法”列出别的中文资料里和本书不同的说法，读别的书时可以对照，“—”表示没有不同的说法。最后一列是易混点和正文出处。

## 线程与同步

| 英文 | 本书用法 | 其他资料常见译法 | 备注/易混 |
|---|---|---|---|
| thread / process | 线程 / 进程 | — | 进程有独立地址空间，线程共享；见 [3.1 节](ch03.md) |
| join / detach | 汇入 / 分离 | 合并（join）；回收（`pthread_join`）/ 脱离 | `join()`/`detach()`，pthread 是 `pthread_join`/`pthread_detach`；见 [3.5 节](ch03.md) |
| mutex | 互斥量 | 互斥锁 | 不写“互斥体”；`std::mutex`/`pthread_mutex_t`；见 [4.2 节](ch04.md) |
| condition variable | 条件变量 | — | `std::condition_variable`/`pthread_cond_t`；本身不保护数据，必须配互斥量；见 [7.2 节](ch07.md) |
| monitor | 监视器（用 mutex + condition_variable 实现） | — | 标准库没有 monitor 类型；等价物是 7.3 的线程安全队列；见 [7.3 节](ch07.md) |
| semaphore | 信号量 | — | `std::counting_semaphore`/`sem_t`；没有持有者；见 [8.1 节](ch08.md) |
| barrier | 屏障 | 路障；栅栏（和 fence 混用） | `std::barrier`、`pthread_barrier_*`、OpenMP `barrier`；见 [8.3 节](ch08.md) |
| latch | 闩 | 锁存器 | `std::latch` 一次性计数，到 0 不能重置；与 barrier 区分；见 [8.3 节](ch08.md) |
| spurious wakeup / lost wakeup | 虚假唤醒 / 丢失唤醒 | 伪唤醒 / — | `wait(lock, pred)` 或 `while` 重查谓词；丢失唤醒是通知早于等待；见 [7.2 节](ch07.md) |
| deadlock / livelock / starvation | 死锁 / 活锁 / 饥饿 | — | 死锁是互相等待；活锁是一直重试；饥饿是长期得不到资源；见 [5.1 节](ch05.md)、[5.5 节](ch05.md) |
| race condition / data race | 竞争条件 / 数据竞争 | 条件竞争、竞态条件 / — | 竞争条件看结果是否依赖顺序；数据竞争是同一位置、至少一个写、无先行发生的 UB；见 [4.1 节](ch04.md)、[11.5 节](ch11.md) |

## 内存与原子

| 英文 | 本书用法 | 其他资料常见译法 | 备注/易混 |
|---|---|---|---|
| atomic operation | 原子操作 | — | `std::atomic<T>`、`std::atomic_ref`；原子性不等于顺序；见 [12.1 节](ch12.md) |
| memory order | 内存序 | — | `std::memory_order` 六种，默认 `seq_cst`；见 [13.1 节](ch13.md) |
| happens-before / synchronizes-with | 先行发生 / 同步于 | — | 线程内“序前于”，线程间“同步于”，合起来是先行发生；见 [11.4 节](ch11.md) |
| fence | 栅栏 | 内存栅栏 | `std::atomic_thread_fence`/`atomic_signal_fence`；与 barrier 的“屏障”分开；见 [13.4 节](ch13.md) |
| lock-free / wait-free / obstruction-free | 无锁 / 无等待 / 无阻碍 | — | 三档进展保证，不互换；见 [15.1 节](ch15.md) |
| hazard pointer | 风险指针 | — | C++26 `std::hazard_pointer`，本机未实现；见 [16.3 节](ch16.md) |
| RCU | RCU | — | read-copy update；C++26 `<rcu>`，本机未实现；见 [16.5 节](ch16.md) |
| false sharing / cache line | 伪共享 / 缓存行 | — / 高速缓存行 | 同一缓存行的不同变量互相失效；`std::hardware_destructive_interference_size` 本机 64；见 [2.5 节](ch02.md)、[18.3 节](ch18.md) |

## 并行与性能

| 英文 | 本书用法 | 其他资料常见译法 | 备注/易混 |
|---|---|---|---|
| concurrency / parallelism | 并发 / 并行 | 常把两者混用 | 并发是程序结构，并行是同时执行；见 [1.1 节](ch01.md) |
| speedup | 加速比 | — | `S = T_串行 / T_并行`；见 [18.1 节](ch18.md) |
| efficiency | 效率 | — | `E = S / p`，理想线性为 1；见 [18.1 节](ch18.md) |
| strong / weak scaling | 强可扩展 / 弱可扩展 | 可扩展性（不分强弱） | 规模固定 vs 规模随 p 同倍率增长；见 [18.1 节](ch18.md) |
| Amdahl | Amdahl 定律 | 阿姆达尔定律 | `S ≤ 1/(r+(1-r)/p)`，上限 `1/r`；见 [18.1 节](ch18.md) |
| roofline | roofline（保留英文） | — | 算力上限与带宽上限取小；见 [18.1 节](ch18.md) |
| task | 任务 | — | 工作单元；OpenMP `task` 另有依赖和作用域；见 [17.1 节](ch17.md)、[22.7 节](ch22.md) |
| worksharing | 工作共享（worksharing） | — | OpenMP `for`/`sections`/`single` 等构造，末尾通常有隐式屏障；见 [22.6 节](ch22.md) |
| reduction | 归约 | — | `reduction` 子句、`MPI_Reduce`；合并顺序未指定；见 [22.3 节](ch22.md) |
| schedule | 调度 | — | `static`/`dynamic`/`guided`/`auto`/`runtime`；见 [22.5 节](ch22.md) |
| chunk | 块（chunk） | 任务块；迭代块（`chunksize`） | `schedule(kind[, chunk])` 的块大小；与 block 划分的“块”区分；见 [22.5 节](ch22.md) |

## MPI

| 英文 | 本书用法 | 其他资料常见译法 | 备注/易混 |
|---|---|---|---|
| rank | rank（进程编号） | 进程号 | `MPI_Comm_rank`，0…p−1；见 [29.1 节](ch29.md) |
| communicator | 通信子 | — | `MPI_Comm`、`MPI_COMM_WORLD`；集合通信按通信子和调用顺序匹配；见 [29.2 节](ch29.md) |
| derived datatype | 派生数据类型 | — | `MPI_Type_contiguous`/`MPI_Type_vector`/`MPI_Type_create_struct`，`MPI_Type_commit`；见 [30.4 节](ch30.md) |
| collective | 集合通信 | — | 通信子内所有进程调用同一函数、顺序一致、无 tag；见 [30.1 节](ch30.md) |
| point-to-point | 点对点 | 点对点通信 | `MPI_Send`/`MPI_Recv`；网络编程资料里的“点对点”指 P2P 网络模型，不是 MPI；见 [29.2 节](ch29.md) |
| non-blocking | 非阻塞（MPI） | 非阻塞（也用来说数据结构、I/O，含义不同） | `MPI_Isend`/`MPI_Irecv`，完成前缓冲区不能复用；与 I/O 的非阻塞不同；见 [30.6 节](ch30.md) |
| persistent | 持久（persistent） | — | MPI-4.0 持久集合，`MPI_Bcast_init`/`MPI_Allreduce_init` 配 `MPI_Start`/`MPI_Wait`；本机 Open MPI 报 3.1，但已提供这些接口（40.6 节实测）；见 [30.6 节](ch30.md) |

## I/O 与系统

| 英文 | 本书用法 | 其他资料常见译法 | 备注/易混 |
|---|---|---|---|
| file descriptor（fd） | 文件描述符（fd） | — | `select`/`epoll` 都用 fd 索引；`FD_SETSIZE` 1024；见 [32.1 节](ch32.md)、[32.3 节](ch32.md) |
| level-triggered（LT）/ edge-triggered（ET） | 水平触发（LT）/ 边缘触发（ET） | 电平触发 / 边沿触发 | LT 条件成立就反复报告；ET 只在状态变化时报告，必须读到 `EAGAIN`；见 [32.5 节](ch32.md) |
| thundering herd | 惊群 | — | 多个执行体等同一监听 fd 时全部被唤醒；`EPOLLEXCLUSIVE`（Linux 4.5）减少但不能消除；见 [32.6 节](ch32.md) |
| async-signal-safe | 异步信号安全 | — | POSIX 列表；`write` 可，`malloc`/`printf` 不可；有的资料只用“可重入”描述信号处理函数；与线程安全不同；见 [26.2 节](ch26.md) |
| reentrant / thread-safe | 可重入 / 线程安全 | 常把两者当同义词混用 | 线程安全可用锁；可重入不依赖可变全局/静态状态；见 [10.3 节](ch10.md) |
| futex | futex（保留英文） | — | `FUTEX_WAIT`/`FUTEX_WAKE`；完整语义 Linux 2.6 起；见 [4.5 节](ch04.md) |
| NUMA / SMT | NUMA / SMT | — / 同步多线程 | NUMA 非一致内存访问；SMT 同时多线程（超线程）；见 [2.4 节](ch02.md) |

## 异步与生态

| 英文 | 本书用法 | 其他资料常见译法 | 备注/易混 |
|---|---|---|---|
| future / promise / packaged_task | future / promise / packaged_task | packaged_task 译作“打包任务” | `std::future` 取结果，`std::promise` 设值，`packaged_task` 绑定可调用对象和 future；见 [9.2 节](ch09.md) |
| executor | executor（保留英文） | — | 决定 handler/任务在哪个线程或执行资源上跑；C++26 的调度器叫 `scheduler`；见 [34.4 节](ch34.md)、[39.6 节](ch39.md) |
| sender/receiver | sender/receiver（保留英文） | — | C++26 `std::execution`；`connect`/`start`，完成走 `set_value`/`set_error`/`set_stopped`；本机未实现；见 [39.6 节](ch39.md) |
| coroutine | 协程 | — | `co_await`/`co_return`；恢复线程由 executor 决定；见 [36.4 节](ch36.md) |

## 五个最容易混

- **barrier / latch**：屏障可复用，闩一次性；`std::barrier` 每轮重置，`std::latch` 到 0 后不能再用。见 [8.3 节](ch08.md)。
- **monitor / condition variable**：监视器是“对象 + 互斥量 + 条件变量”的封装；条件变量只是等待/通知原语，本身不保护数据，标准库没有 monitor 类型。见 [7.2 节](ch07.md)、[7.3 节](ch07.md)。
- **race condition / data race**：竞争条件看结果是否依赖执行顺序；数据竞争是语言层面的未定义行为，判据是同一内存位置、至少一个写、没有先行发生关系。见 [11.5 节](ch11.md)。
- **lock-free / wait-free**：无锁保证系统整体有进展，无等待保证每个线程有限步完成；还有更弱的无阻碍。三者不互换。见 [15.1 节](ch15.md)。
- **reentrant / thread-safe**：线程安全可以用锁；可重入要求不依赖可变全局/静态状态，能在被打断后再次进入。见 [10.3 节](ch10.md)。

## 延伸阅读

- [C++ 草案](https://eel.is/c++draft/)：标准术语和条款编号。
- [POSIX.1-2024](https://pubs.opengroup.org/onlinepubs/9799919799/)：Base Specifications 的术语表。
- [OpenMP 5.2 规范](https://www.openmp.org/specifications/)：术语表和 worksharing、`schedule` 的定义。
- [MPI 4.1 规范](https://www.mpi-forum.org/docs/)：通信子、集合通信和持久集合的术语。
- [`pthreads(7)`](https://man7.org/linux/man-pages/man7/pthreads.7.html)：POSIX 线程接口概述。
