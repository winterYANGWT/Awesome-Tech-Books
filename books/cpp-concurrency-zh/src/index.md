# 前言

## 这本书讲什么、写给谁

这本书讲一个进程里的线程怎样共享数据、怎样少犯错，再讲到多核并行、多进程、多机和同时服务大量连接。主线是 C++20 标准库；pthread 与它并排对照，OpenMP、MPI 各有一条线。

写给会写 C++（至少 C++11）、能读 C、熟悉 Linux 命令行的开发者。不需要学过并发；指针、进程、TCP 连接这些前置概念不从头讲，缓存一致性、内存序、futex、epoll 会讲清楚。

## 全书路线图

十个部分按“线程 → 同步 → 内存模型 → 数据结构 → 设计 → 并行计算 → 进程 → 多机 → 服务器 → 回看旧写法”展开。箭头是主要依赖；同一层的部分可以换顺序。第十部分在最后，逐个回看前九部分。

{{#fig preface/roadmap}}

可以按想解决的问题找入口：

| 想学什么 | 读哪几部分 |
|---|---|
| 写正确的多线程程序 | 第一、二部分；结果不对时查第三部分 |
| 让计算在多核上跑满 | 第五、六部分（先读第二、五部分） |
| 写高并发服务器 | 第七、九部分（先读第二、五部分） |
| 把计算扩到多台机器 | 第五、八部分 |
| 看懂旧代码为什么这样写 | 第十部分，按案例回查前面 |

## 四套 API 和三个贯穿示例

四套 API 的分工：

| API | 讲在哪几部分 | 本书怎么用 |
|---|---|---|
| C++ 标准库 | 第一至第六部分、第九部分 | 主线：线程和同步设施先用它写，再给 pthread 对照 |
| pthread / POSIX | 第二、三、五、七部分 | 每个基本设施并排给出 C 版，比较错误码、属性、取消和超时 |
| OpenMP | 第六部分 | 规则循环、归约和调度；和 C++17 并行算法对照 |
| MPI | 第八部分 | 多进程和分布式内存：点对点、集合通信、派生类型与性能 |

三个示例各贯穿一条线：

| 示例 | 出现的章 | 怎么演进 |
|---|---|---|
| 数值归约：`sum`、`pi`、`matvec` | 3、4、8、12、17、18、22、23、29、30 | 各线程写自己的结果槽 → 共享累加器（丢失更新、互斥量、私有累加）→ 屏障分阶段 → 原子累加 → 划分方式 → 伪共享 → OpenMP 归约 → `std::reduce(par)` → MPI 归约和按行/列分布 |
| 任务队列 | 7、14、15、16、19、20、28 | 一把锁的 `ThreadSafeQueue` → 头尾两把锁 → 无锁栈和 SPSC 环形队列 → 风险指针、引用计数、epoch 回收 → 线程池和工作窃取 → `stop_token` 关闭 → actor 邮箱 |
| 键值服务 `kvd` | 25、27、32–36 | 每连接一个 fork 子进程 → prefork 进程池加共享内存 → 迭代版 → 每连接一个线程 → epoll Reactor → Libevent/Asio → 半同步/半反应堆、`SO_REUSEPORT` → io_uring 和协程 |

## 版本基准

正文按下面这套版本写；提到 Linux 专有接口时，另行注明起始内核版本。

| 项目 | 本书基准 | 读者要知道的 |
|---|---|---|
| C++ | C++20（`g++-15 -std=c++20`） | `jthread`、`stop_token`、`latch`/`barrier`、`atomic::wait` 都在 C++20；C++23 的例子单独标 `-std=c++23` |
| C | C17（`gcc -std=c17`） | pthread 示例，以及 C11 原子和 `<threads.h>` 的对照 |
| Linux / glibc | Linux 7.0.0，glibc 2.43 | 每个 Linux 专有接口注明起始内核版本 |
| 编译器 | GCC 15.2，Clang 21.1.8 | 默认用 GCC；需要 Clang 的例子单独注明 |
| OpenMP | GCC 15 的 libgomp（4.5） | `_OPENMP` 为 201511；5.x 特性逐一注明编译器，libomp 报 5.1 |
| MPI | Open MPI 5.0.10（报 MPI 3.1） | 持久集合等部分 MPI-4.0 接口已经能用（第 40 章实测），其余 4.x 接口只介绍 |

## 怎么读示例和图

示例都实际编译运行过，输出由脚本生成，正文直接引用。多线程示例再在 ThreadSanitizer（TSan）下跑一遍；故意写错的示例分几类，看到的东西不一样。

| 示例类型 | 你会看到什么 |
|---|---|
| 数据竞争 | TSan 报告：两处冲突访问、文件名和行号，以及 `data race` |
| 崩溃 | 断言或 `std::terminate` 的输出，程序以非 0 退出或被信号终止 |
| 卡死 | 到时限后用 gdb 取到的各线程栈，停在哪一行等锁或等条件变量 |
| 编译失败 | GCC 的诊断：文件名、行号、错误原因，以及相关的声明 |

计时数字随机器和负载变化，正文只讲趋势和数量级，具体数看输出和图。本机是 AMD Ryzen 9 9955HX（16 核 32 线程）、Linux 7.0.0、glibc 2.43，用 GCC 15 编译。动图可以播放、暂停、单步前进和后退；下面这张图单步到第 2、4 步，看 B 读到的 0 和写回的 1。

{{#fig preface/lost_update}}

## 示例代码怎么获取和编译

{{#fig preface/build}}

每章的示例在 `code/chNN/` 下，按小节编号命名；服务器、gdb、MPI 等多文件示例放在子目录里，带 `example.sh`。下面在 `src/` 目录下执行，可执行文件放到别处。数据竞争类的示例要加 `-fsanitize=thread` 才看得到问题：不加的话，`-O2` 可能把循环优化掉，结果碰巧是对的。

```bash
out=/tmp/ccbook && mkdir -p $out

# C++：第 4 章用互斥量求和
g++-15 -std=c++20 -O2 -Wall -Wextra -pthread code/ch04/4_2_mutex_sum.cpp -o $out/4_2
$out/4_2

# 数据竞争的示例用 TSan 编译，看它的报告
g++-15 -std=c++20 -O1 -g -fsanitize=thread -pthread code/ch04/4_1_lost_update.cpp -o $out/4_1
$out/4_1

# C：pthread 示例用 gcc
gcc -std=c17 -O2 -Wall -Wextra -pthread code/ch04/4_2_pthread_mutex.c -o $out/4_2c
$out/4_2c

# OpenMP 加 -fopenmp
gcc -std=c17 -O2 -Wall -Wextra -fopenmp code/ch22/22_1_hello.c -o $out/22_1
$out/22_1

# MPI 用 mpicc 编译，mpirun 启动多个副本
mpicc -std=c17 -O2 -Wall -Wextra code/ch29/29_1_hello/hello.c -o $out/29_1
mpirun -np 4 $out/29_1
```
