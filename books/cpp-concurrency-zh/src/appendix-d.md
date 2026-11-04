# 附录 D 工具与编译选项

先按症状查，再按用途找命令；每行给最小命令、看哪一行或哪个字段，以及本机限制。

## 症状 → 工具

| 症状 | 先看 | 去哪 |
|---|---|---|
| 程序卡住 | gdb `thread apply all bt` | [21.6 节](ch21.md)、[5.6 节](ch05.md) |
| 数据竞争 | TSan | [21.5 节](ch21.md) |
| 释放后使用 | ASan | [16.1 节](ch16.md)、[21.6 节](ch21.md) |
| 系统调用失败 | `strace` / `errno` | [5.6 节](ch05.md)、[32.7 节](ch32.md) |
| MPI 启动失败 | `mpirun` 环境变量 | [29.1 节](ch29.md) |
| OpenMP 线程数不对 | `OMP_DISPLAY_ENV` / `_OPENMP` | [22.1 节](ch22.md) |

本机：GCC 15.2、Clang 21.1.8、gdb 17.1、Open MPI 5.0.10、libevent 2.1.12、liburing 2.14、TBB 2022.3、libomp 21（`libomp-21-dev`）；没有 `perf`、`valgrind`、`strace`、`lsof`、`ss`，`man` 没有页；`numactl`、`taskset`、`gcore` 可用；`io_uring_queue_init` 在本机成功。

## 工具与编译选项

| 目的 | 工具/选项 | 最小命令 | 看什么 | 本机可用性/限制 | 对应章 |
|---|---|---|---|---|---|
| **编译**：C++ 基线 | `g++-15`、`-std=c++20 -O2 -g -Wall -Wextra -Wpedantic -Werror -pthread` | `g++-15 -std=c++20 -O2 -g -Wall -Wextra -Wpedantic -Werror -pthread app.cpp -o a.out` | 命令行里的 `-std=` 和 `-pthread`；失败时第一个 `error:` 的 `file:line` | GCC 15.2 可用 | [1.5 节](ch01.md)、[37.1 节](ch37.md) |
| C 基线 | `gcc`、`-std=c17`、`-pthread` | `gcc -std=c17 -O2 -g -Wall -Wextra -Wpedantic -Werror -pthread app.c -o a.out` | 第一个 `error:` 的行号；链接时 `undefined reference` 的符号 | GCC 15.2 可用；C 文件用 `gcc`，不用 `g++` | [1.5 节](ch01.md) |
| Clang 21 / C++23 | `clang++-21`、`clang-21`、`-std=c++23` | `clang++-21 -std=c++23 -O2 -g -Wall -Wextra -Wpedantic -Werror -pthread app.cpp -o a.out` | `clang++-21 --version` 的 `21.1.8`；C++23 特性的诊断行 | Clang 21.1.8 可用，只有 libstdc++ 15，没有 libc++；GCC 15.2 也支持 `-std=c++23` | [1.5 节](ch01.md)、[22.1 节](ch22.md) |
| OpenMP | `-fopenmp` | `g++-15 -std=c++20 -O2 -g -Wall -Wextra -Wpedantic -Werror -fopenmp app.cpp -o a.out` | 编译行有没有 `-fopenmp`；`omp.h` 能否找到；C 程序用 `gcc` 加同一选项 | GCC 15.2（libgomp，`_OPENMP=201511`）和 Clang 21.1.8（libomp 21，`202011`）都可用 | [22.1 节](ch22.md) |
| TBB / libatomic | `-ltbb`、`-latomic` | `g++-15 -std=c++20 -O2 -g -Wall -Wextra -Wpedantic -Werror -pthread app.cpp -ltbb -latomic -o a.out` | 未定义引用里的 `tbb::detail::r1::` 或 `__atomic_*_16` | TBB 2022.3 可用；GCC 15.2 的 16 字节原子即使加 `-mcx16` 仍非无锁，链接要 `-latomic` | [23.1 节](ch23.md)、[12.6 节](ch12.md) |
| Libevent | `-levent`、`-levent_pthreads` | `gcc -std=c17 -O2 -g -Wall -Wextra -Wpedantic -Werror -pthread ev.c -levent -levent_pthreads -o ev` | `event_base_get_method` 的返回值；`evthread_use_pthreads()` 是否返回 0 | libevent 2.1.12 可用 | [34.3 节](ch34.md)、[42.7 节](ch42.md) |
| liburing | `-luring` | `gcc -std=c17 -O2 -g -Wall -Wextra -Wpedantic -Werror uring.c -luring -o uring` | `io_uring_queue_init` 的返回值；报 `sigset_t`/`AT_FDCWD` 未定义时在首行加 `#define _GNU_SOURCE` | liburing 2.14 可用；`io_uring_queue_init` 本机返回 0 | [36.2 节](ch36.md) |
| **Sanitizer**：TSan | `-fsanitize=thread` | `g++-15 -std=c++20 -O1 -g -fsanitize=thread -pthread app.cpp -o app.tsan && ./app.tsan` | 第一条 `#0 file:line`（当前访问）、`Previous write` 的 `#0`、`SUMMARY` 行、`Location is global`/`on stack` | GCC 15.2 / Clang 21.1.8 都可用，报告带 `file:line` | [21.5 节](ch21.md) |
| ASan / UBSan | `-fsanitize=address,undefined` | `g++-15 -std=c++20 -O1 -g -fsanitize=address,undefined -pthread app.cpp -o app.asan && ./app.asan` | `ERROR: AddressSanitizer: heap-use-after-free` 的 `READ/WRITE` 行、`freed by thread` 的 `#0`、UBSan 的 `runtime error:` 行 | GCC 15.2 / Clang 21.1.8 可用；本机没有 `valgrind`，Helgrind/DRD 用不了 | [16.1 节](ch16.md)、[21.6 节](ch21.md) |
| libgomp 未插桩 | GCC `-fopenmp` + `-fsanitize=thread` | `g++-15 -std=c++20 -O1 -g -fsanitize=thread -fopenmp app.cpp -o app.tsan && ./app.tsan` | 报告的 `#0` 是否落在 `libgomp.so` 或 `GOMP_parallel`，而不是源码行 | GCC 15.2 的 libgomp 未插桩，会误报；OpenMP 示例改 Clang 21.1.8 + libomp 21，或写 `// tsan: no` | [21.5 节](ch21.md)、[22.4 节](ch22.md) |
| **gdb**：启动 | `gdb --args` | `gdb -q --args ./a.out` | `(gdb)` 提示符；`run` 后停住的 `#0 file:line` | gdb 17.1 可用 | [21.6 节](ch21.md) |
| 线程列表与全栈 | `info threads`、`thread N`、`thread apply all bt` | `gdb -q -p PID -batch -ex "set pagination off" -ex "info threads" -ex "thread apply all bt"` | `Id` 列、`*` 当前线程、`LWP`；每个 `Thread N` 的 `#0`；锁等待时 `pthread_mutex_lock` 上一帧的 `file:line` | gdb 17.1 可用；`-p` 需要 ptrace 权限 | [21.6 节](ch21.md)、[5.6 节](ch05.md) |
| 单步与 fork 跟随 | `set scheduler-locking on/off`、`set follow-fork-mode child/parent`、`set detach-on-fork off` | `gdb -q -ex "break main" -ex run -ex "set scheduler-locking on" -ex "set follow-fork-mode child" -ex "set detach-on-fork off" -ex continue ./a.out` | 单步时其他线程是否停；`info inferiors` 里父子两个 inferior，`*` 是当前 | gdb 17.1 可用；`follow-fork-mode` 默认 `parent` | [21.6 节](ch21.md)、[41.4 节](ch41.md) |
| 抓 fork / clone | `catch fork`、`catch syscall clone`、`catch syscall clone3` | `gdb -q -ex "catch fork" -ex "catch syscall clone clone3" -ex run ./a.out` | `Catchpoint N (fork)` 或 `(call to syscall clone3)`；停住后 `bt` 顶部是 `fork` 或 `pthread_create` | gdb 17.1 可用；glibc 2.43 的 `pthread_create` 走 `clone3`，只写 `clone` 抓不到 | [21.6 节](ch21.md) |
| 寄存器 | `info registers` | `gdb -q -ex "break main" -ex run -ex "info registers" ./a.out` | `rip`、`rsp`、`rdi`/`rsi`/`rdx`、`rax`；崩溃时先看 `rip` 和 `rsp` | gdb 17.1 可用 | [21.6 节](ch21.md) |
| core 文件 | `gcore`、`gdb ./a.out core` | `gcore -o core PID; gdb -q ./a.out core.PID -ex "thread apply all bt"` | core 里的 `Thread N` 和 `#0`；`info registers` 的 `rip`/`rsp` | `gcore` 可用；需要 ptrace 权限；core 大小受 `ulimit -c` 影响 | [5.6 节](ch05.md)、[21.6 节](ch21.md) |
| **运行观测**：线程枚举 | `/proc/PID/task`、`ps -L`、`top -H` | `ps -L -p PID -o pid,tid,comm,stat` | `TID`、`STAT`、`COMMAND`；`/proc/PID/task` 的目录项与 `top -H` 的每行 | 可用；`TID` 是内核线程号 | [3.1 节](ch03.md) |
| fd 表 | `/proc/PID/fd` | `ls -l /proc/PID/fd` | fd 编号和 `->` 目标；`socket:[inode]` 的 inode 与 `/proc/net/tcp` 对上 | 可用；和 `lsof` 看的是同一张表 | [3.1 节](ch03.md)、[32.1 节](ch32.md) |
| 进程/线程状态 | `/proc/PID/status` | `cat /proc/PID/status` | `State:`、`Threads:`、`voluntary_ctxt_switches:`、`nonvoluntary_ctxt_switches:` | 可用 | [3.1 节](ch03.md) |
| 拓扑与绑核 | `lscpu`、`nproc`、`taskset -c`、`numactl --hardware` | `lscpu; nproc; taskset -c 0,1 nproc; numactl --hardware` | `CPU(s)`、`Thread(s) per core`、`Core(s) per socket`、`NUMA node(s)`；`nproc` 从 32 变 2；`node 0 cpus`/`size` | `lscpu`、`nproc`、`taskset`、`numactl` 可用；本机 16 核 32 线程、1 个 NUMA 节点 | [2.4 节](ch02.md)、[18.4 节](ch18.md) |
| 线程名与 TID | `pthread_setname_np`/`pthread_getname_np`、`/proc/self/task/TID/comm`、`gettid` | `cat /proc/PID/task/TID/comm` | `comm` 的名字与 `ps -L` 的 `COMMAND` 对齐；`gettid()` 的值等于目录名 `TID` | glibc 2.12 起；名字最多 16 字节含 NUL；本机可用 | [3.6 节](ch03.md)、[19.6 节](ch19.md) |
| **网络与系统调用**：监听端口 | `ss -ltnp` | `ss -ltnp` | `LISTEN` 行的 `Local Address:Port`、`users:(("name",pid=...))` | 本机没装 `ss`；改看 `awk '$4=="0A"' /proc/net/tcp` 的 `st=0A` 行和 `inode` | [32.1 节](ch32.md)、[42.1 节](ch42.md) |
| 进程打开的 fd | `lsof -nP -p PID` | `lsof -nP -p PID` | `FD`、`TYPE`、`NAME` 三列；`TCP *:port` 是监听 fd | 本机没装 `lsof`；用 `ls -l /proc/PID/fd` 代替 | [32.1 节](ch32.md)、[42.1 节](ch42.md) |
| 系统调用跟踪 | `strace -f` | `strace -f ./a.out` | 最后一行的 `= -1 E...` 和 `errno`；`futex` 等待行 | 本机没装 `strace`；容器里常受 `ptrace` 限制 | [5.6 节](ch05.md)、[32.7 节](ch32.md) |
| eBPF 跟踪 | `bpftrace` | `bpftrace -e 'tracepoint:syscalls:sys_enter_futex { @[comm] = count(); }'` | `@[comm]` 的计数；哪个线程名在等 `futex` | 本机没装 `bpftrace`；需要 BTF 和权限 | [5.6 节](ch05.md) |
| 采样与计数器 | `perf` | `perf stat -d ./a.out` | `cycles`、`instructions`、`cache-misses`、`context-switches`；`perf report` 的热点 `#0` | 本机没有 `perf`，`perf_event_paranoid=4` | [18.3 节](ch18.md)、[5.6 节](ch05.md) |
| **MPI**：编译 | `mpicc`、`mpicxx` | `mpicc -std=c17 -O2 -Wall -Wextra hello.c -o hello` | 包装脚本里的 `-I.../openmpi/include`、`-L... -lmpi`；`MPI_Init` 返回 0 | Open MPI 5.0.10 可用；C++ 用 `mpicxx` | [29.1 节](ch29.md) |
| 启动 | `mpirun -np 4` | `mpirun -np 4 ./hello` | 是否有 4 行 `rank 0..3 of 4`；`-np` 是进程数，不是线程数 | Open MPI 5.0.10；root 下直接报 `attempt to run as root` | [29.1 节](ch29.md) |
| root / 沙箱 | `OMPI_ALLOW_RUN_AS_ROOT=1`、`OMPI_ALLOW_RUN_AS_ROOT_CONFIRM=1`、`--oversubscribe` | `OMPI_ALLOW_RUN_AS_ROOT=1 OMPI_ALLOW_RUN_AS_ROOT_CONFIRM=1 mpirun --oversubscribe -np 4 ./a.out` | root 警告消失，出现 4 行 rank；沙箱里 `PRTE ERROR` 指向 `odls_base_default_fns.c` 就是文件句柄限制 | Open MPI 5.0.10；`--oversubscribe` 用于进程数超过核数；dsh 沙箱里起不来，由负责人在沙箱外运行 | [29.1 节](ch29.md) |
| **OpenMP**：线程数与调度 | `OMP_NUM_THREADS`、`OMP_SCHEDULE` | `OMP_NUM_THREADS=4 OMP_SCHEDULE='dynamic,2' ./app` | `omp_get_max_threads()` / `omp_get_num_threads()` 是否 4；`omp_get_schedule` 的 kind 和 chunk；`num_threads` 子句优先于环境变量 | GCC 15.2 / Clang 21.1.8 可用 | [22.1 节](ch22.md)、[22.5 节](ch22.md) |
| 绑核 | `OMP_PROC_BIND`、`OMP_PLACES` | `OMP_PROC_BIND=close OMP_PLACES=cores OMP_DISPLAY_AFFINITY=TRUE ./app` | `affinity` 行，或 `/proc/TID/status` 的 `Cpus_allowed_list`；线程是否落在不同物理核 | GCC 15.2 / Clang 21.1.8 可用；具体映射由实现决定 | [22.5 节](ch22.md)、[2.4 节](ch02.md) |
| 环境与版本宏 | `OMP_DISPLAY_ENV=TRUE`、`_OPENMP` | `OMP_DISPLAY_ENV=TRUE ./app; gcc -fopenmp -dM -E -x c /dev/null` | `OMP_NUM_THREADS`、`OMP_SCHEDULE`、`OMP_PROC_BIND`、`OMP_PLACES` 四行；第二条命令输出里的 `#define _OPENMP`（把 `gcc` 换成 `clang-21` 得到另一组值）；`OMP_DISPLAY_ENV` 末尾的 `_OPENMP` 是运行时版本，不是宏 | GCC 15.2 的宏是 `201511`，Clang 21.1.8 的 libomp 是 `202011` | [22.1 节](ch22.md)、[40.2 节](ch40.md) |
| **其他**：资源上限 | `ulimit -n`、cgroup v2 | `ulimit -n; cat /sys/fs/cgroup/pids.max /sys/fs/cgroup/cpu.max` | `ulimit -n` 的软限制（本机 524288）；`pids.max` 17104；`cpu.max` 的 `配额 周期`（本机 `max 100000`） | 可用；`ulimit -Hn` 看硬限制 | [42.1 节](ch42.md)、[35.1 节](ch35.md) |
| 页大小 | `getconf PAGESIZE` | `getconf PAGESIZE` | 4096；栈保护区、`mmap` 对齐都用它 | 本机 4096 | [3.6 节](ch03.md)、[2.1 节](ch02.md) |
| libpthread 是否已并入 libc | `ldd` | `ldd ./a.out` | 输出里有没有 `libpthread.so.0`；glibc 2.34 起没有 | 本机 glibc 2.43，`libpthread` 已并入 `libc.so.6`；`-pthread` 仍要写 | [37.1 节](ch37.md) |

> **旧资料提示**：`netstat` 已被 `ss` 和 `/proc/net/tcp` 取代（iproute2）；本机 `ss` 也没装，直接看 `/proc/net/tcp`。`strace` 在容器里常被 `ptrace` 限制，`perf trace`/`bpftrace` 要另核实——本机两者都没有。gdb 的 `follow-fork-mode` 仍是当前用法，但要配合 `detach-on-fork off` 和 `catch fork`，见 [21.6 节](ch21.md)、[41.4 节](ch41.md)。

## 速查

```bash
g++-15 -std=c++20 -O2 -g -Wall -Wextra -Wpedantic -Werror -pthread app.cpp -o a.out && ./a.out
g++-15 -std=c++20 -O1 -g -fsanitize=thread -pthread app.cpp -o app.tsan && ./app.tsan
timeout -s INT 3 gdb -q -batch -ex "set pagination off" -ex run -ex "thread apply all bt" ./a.out
```

## 延伸阅读

- GDB 官方手册（gdb 17.1）：`info threads`、`thread apply all bt`、`scheduler-locking`、`follow-fork-mode`；本机 `refs/man-pages` 没有 gdb 页，不给链接。
- `strace(1)`：系统调用、参数和 `errno`；本机未安装，不给链接。
- `perf(1)`：`perf stat`、`perf record`；本机不可用（`perf_event_paranoid=4`），不给链接。
- GCC 的 `-fsanitize=thread` 文档：https://gcc.gnu.org/onlinedocs/gcc/Instrumentation-Options.html
- Open MPI 5.0.10 的 `mpirun(1)`：https://docs.open-mpi.org/
- OpenMP 5.2 规范的环境变量章节：https://www.openmp.org/specifications/
- ThreadSanitizer 报告格式：https://clang.llvm.org/docs/ThreadSanitizer.html
