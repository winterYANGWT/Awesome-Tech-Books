# 附录 A API 对照总表

先按任务找行，横向看六套接口，最后读差别。`无` 指正文没给出，`—` 指没有对应项；最后一组是编译选项。Linux 的 libstdc++/libc++ 建在 pthread 上，MSVC 不是。

| 任务 | C++ 标准库（C++20/23） | pthread/POSIX | C11 | OpenMP | MPI | Linux 系统调用 | 关键差别 |
|---|---|---|---|---|---|---|---|
| 线程生命周期：创建、汇入、分离 [3.2 节](ch03.md)、[3.5 节](ch03.md) | `thread`、`jthread`、`join`、`detach`、`joinable` | `pthread_create`、`pthread_join`、`pthread_detach` | `thrd_create`、`thrd_join` | `parallel` | `mpirun` | `clone`、`clone3` | C++ 构造即启动，`std::thread` 析构时仍 joinable 会 `terminate`，`jthread` 析构先请求停止再汇入；OpenMP 在区域末尾隐式汇合，MPI 的“线程”是 `mpirun` 启动的进程。 |
| 线程生命周期：线程 id [3.7 节](ch03.md) | `thread::id`、`get_id`、`this_thread::get_id` | `pthread_self`、`pthread_equal` | `无` | `omp_get_thread_num` | `MPI_Comm_rank` | `gettid`、`/proc/self/task` | `pthread_t` 不透明，`thread::id` 可比较和哈希；rank 是进程编号，不是线程 id。 |
| 线程生命周期：睡眠与让出 [9.4 节](ch09.md)、[19.2 节](ch19.md) | `sleep_for`、`sleep_until`、`yield` | `nanosleep` | `无` | `无` | `无` | `clock_nanosleep` | 睡眠只保证至少等这么久，不能当同步；`yield` 只是让出调度机会，不保证别人运行。 |
| 互斥量与锁：独占与递归 [4.2 节](ch04.md)、[4.6 节](ch04.md) | `mutex`、`recursive_mutex`、`lock_guard`、`unique_lock` | `pthread_mutex_*`、`PTHREAD_MUTEX_RECURSIVE` | `mtx_*`、`mtx_recursive` | `critical`、`omp_nest_lock_t` | `无` | `futex` | C++ 用 RAII 和异常报告，pthread/C11 手动配对并返回错误码；递归锁的加解锁次数必须配平。 |
| 互斥量与锁：读写锁 [6.1 节](ch06.md) | `shared_mutex`、`shared_timed_mutex`、`shared_lock` | `pthread_rwlock_*` | `无` | `无` | `无` | `—` | C++17 起有 `shared_mutex`，没有优先级旋钮；POSIX 可用属性设读者/写者偏好，glibc 的 `PREFER_WRITER_NP` 被忽略。 |
| 互斥量与锁：一次锁多个 [5.2 节](ch05.md) | `scoped_lock`、`lock`、`try_lock` | `无` | `无` | `无` | `无` | `—` | `scoped_lock` 的参数个数在编译期固定，内部用 `std::lock` 避免死锁；POSIX 没有对应接口，只能自己定加锁顺序。 |
| 互斥量与锁：限时锁 [9.4 节](ch09.md) | `timed_mutex`、`try_lock_for`、`try_lock_until` | `pthread_mutex_timedlock` | `mtx_timedlock` | `无` | `无` | `futex` | 超时返回 false 或各自的超时错误码，只说明没拿到锁，不说明条件成立；绝对时间还要选对时钟。 |
| 条件变量：等待与通知 [7.2 节](ch07.md)、[37.8 节](ch37.md) | `condition_variable`、`condition_variable_any`、`wait`、`notify_one`、`notify_all` | `pthread_cond_*` | `cnd_t`、`cnd_init`、`cnd_signal`、`cnd_wait` | `无` | `无` | `futex` | 条件变量没有记忆，必须用谓词循环防丢失唤醒和虚假唤醒；OpenMP 没有条件变量，只能轮询、用任务或屏障。 |
| 条件变量：可中断等待 [7.5 节](ch07.md)、[20.3 节](ch20.md) | `condition_variable_any`、`stop_token`、`stop_callback` | `pthread_cond_wait` | `cnd_wait` | `无` | `无` | `futex`、`eventfd`、`pipe2` | C++ 的停止请求通过回调唤醒等待，pthread 靠取消点；两者返回后都要重新检查谓词。 |
| 信号量、闩、屏障：信号量 [8.1 节](ch08.md) | `counting_semaphore`、`binary_semaphore`、`acquire`、`release` | `sem_*` | `无` | `无` | `无` | `futex`、`eventfd` | 信号量没有持有者，任何线程都能 `release`；C++ 信号量只有进程内语义，跨进程要用命名信号量。`eventfd` 只是计数通知，不是信号量。 |
| 信号量、闩、屏障：闩 [8.3 节](ch08.md) | `latch`、`count_down`、`wait`、`arrive_and_wait` | `无` | `无` | `无` | `无` | `—` | 闩一次性，计数到 0 后永久就绪；要重复使用就换 `barrier`。 |
| 信号量、闩、屏障：屏障 [8.3 节](ch08.md)、[8.4 节](ch08.md) | `barrier`、`arrive_and_wait`、`arrive_and_drop` | `pthread_barrier_*` | `无` | `barrier` | `MPI_Barrier` | `futex` | `std::barrier` 是可复用的线程级屏障，参与线程数固定；`MPI_Barrier` 是进程级集合操作，所有 rank 都要到。 |
| 一次性初始化与 TLS：一次性初始化 [6.4 节](ch06.md) | `call_once`、`once_flag` | `pthread_once`、`PTHREAD_ONCE_INIT` | `call_once` | `无` | `无` | `futex` | `call_once` 在初始化函数抛异常时不置位，下次重试；`pthread_once` 没有异常，初始化函数不能抛出。 |
| 一次性初始化与 TLS：每线程变量 [10.2 节](ch10.md)、[10.5 节](ch10.md) | `thread_local` | `pthread_key_*` | `_Thread_local`、`tss_*` | `threadprivate` | `无` | `__thread` | `thread_local` 由语言管理构造和析构；pthread key 与 `tss_t` 是运行期动态键，只对非空值调析构，可能多轮。 |
| 一次性初始化与 TLS：线程退出清理 [10.5 节](ch10.md)、[20.4 节](ch20.md) | `thread_local` 析构、RAII | `pthread_key_create` 析构、`pthread_cleanup_push`、`pthread_cleanup_pop` | `tss_create` 析构 | `无` | `无` | `—` | 正常退出和取消走不同清理路径；`pthread_cleanup_*` 只在取消或 `pthread_exit` 时执行，正常 `return` 不执行。 |
| 原子与内存序：原子对象与等待 [12.1 节](ch12.md)、[12.5 节](ch12.md) | `atomic<T>`、`atomic_ref`、`atomic_flag`、`atomic<shared_ptr<T>>`、`wait`、`notify_one` | `无` | `<stdatomic.h>`、`atomic_int`、`atomic_load`、`atomic_store`、`atomic_fetch_add` | `atomic` | `无` | `futex`、`__atomic_*` | C++ 的 `atomic<T>` 可带逐操作内存序，`atomic_flag` 保证无锁，`atomic<shared_ptr<T>>` 不保证无锁；C11 用 `_Atomic`/`atomic_*`，OpenMP 只覆盖有限语句，不能 CAS。`hazard_pointer`/`rcu`（C++26，未实现）是安全回收设施，不是原子类型，正文见 [16.3 节](ch16.md)、[16.5 节](ch16.md)。 |
| 原子与内存序：内存序 [13.2 节](ch13.md)、[13.3 节](ch13.md) | `memory_order_*` | `无` | `memory_order_*` | `atomic`、`flush` | `MPI_Win_flush`、`MPI_Win_sync`、`MPI_Win_fence` | `mfence`、`__atomic_*` | C++ 和 C11 用同一套六种内存序；OpenMP `flush` 只覆盖团队内共享数据，MPI 的窗口同步是远程内存可见性，不是本地 `memory_order`。 |
| 原子与内存序：栅栏 [13.4 节](ch13.md) | `atomic_thread_fence`、`atomic_signal_fence` | `无` | `atomic_thread_fence`、`atomic_signal_fence` | `flush` | `MPI_Win_fence` | `mfence` | 栅栏只排序，不执行操作；`atomic_signal_fence` 只排同线程信号处理函数，不跨线程。 |
| future 与任务：一次性结果 [9.1 节](ch09.md)、[9.2 节](ch09.md) | `future`、`promise`、`shared_future`、`get`、`wait` | `无` | `无` | `无` | `无` | `—` | `future` 是一次性结果槽，`get` 只能取一次；`shared_future` 可拷贝，多读者各取一份。 |
| future 与任务：异步启动 [9.3 节](ch09.md)、[22.7 节](ch22.md) | `async`、`launch::async`、`launch::deferred`；`std::execution`（C++26，未实现） | `无` | `无` | `task`、`taskwait`、`taskgroup` | `无` | `—` | `async` 的启动策略由实现选，future 析构可能阻塞；OpenMP `task` 是结构化任务，异常不能穿出，C++26 sender/receiver 本机未实现。`io_uring` 的 CQ 是 I/O 完成，不是任务 future。 |
| future 与任务：打包任务 [9.2 节](ch09.md)、[19.3 节](ch19.md) | `packaged_task`、`move_only_function`（C++23） | `无` | `无` | `task` | `无` | `—` | `packaged_task` 把可调用对象和 future 绑定，本身不调度；`get_future()` 只能调一次，`move_only_function` 才能直接存它。 |
| 取消与停止：协作停止 [20.2 节](ch20.md)、[20.3 节](ch20.md) | `stop_token`、`stop_source`、`jthread`、`stop_callback` | `无` | `无` | `cancel`（4.0）、cancellation point | `MPI_Cancel` | `pidfd_send_signal` | `request_stop` 只发请求，目标不检查就不会停；pthread 的取消是延迟取消，只在取消点生效，OpenMP `cancel` 也是协作式，`MPI_Cancel` 只取消非阻塞通信请求，`pidfd_send_signal` 只能作用于进程。 |
| 取消与停止：pthread 取消与清理 [20.4 节](ch20.md) | `无` | `pthread_cancel`、`pthread_testcancel`、`pthread_setcancelstate`、`pthread_setcanceltype`、`pthread_cleanup_push`、`pthread_cleanup_pop` | `无` | `cancel`（4.0） | `无` | `—` | 默认延迟取消，异步取消可能在任意指令处留下半更新；C++ 对象在取消下不保证析构，资源要靠清理栈。 |
| 超时与时钟：时钟与计时 [9.4 节](ch09.md)、[18.2 节](ch18.md) | `chrono`、`steady_clock`、`system_clock`、`duration_cast` | `clock_gettime`、`CLOCK_MONOTONIC` | `无` | `omp_get_wtime` | `MPI_Wtime` | `clock_gettime`、`CLOCK_MONOTONIC` | 超时和计时用单调时钟，`system_clock` 会被校时；`MPI_Wtime` 只适合同一 rank 内取差值，跨 rank 时钟不同步。 |
| 超时与时钟：定时等待 [7.2 节](ch07.md)、[9.4 节](ch09.md) | `wait_for`、`wait_until`、`try_lock_for`、`try_lock_until`、`sleep_for`、`sleep_until` | `pthread_cond_timedwait`、`sem_timedwait`、`pthread_mutex_timedlock` | `mtx_timedlock` | `无` | `无` | `clock_nanosleep`、`futex` | `_for` 收时长、`_until` 收时间点；pthread/C11 多用绝对 `timespec`，还要选对 `CLOCK_REALTIME`/`CLOCK_MONOTONIC`。 |
| 线程属性与亲和：属性与栈 [3.6 节](ch03.md) | `native_handle` | `pthread_attr_*`、`pthread_attr_setstacksize`、`pthread_attr_setguardsize` | `无` | `OMP_STACKSIZE` | `无` | `RLIMIT_STACK` | 标准库没有属性对象，只能 `native_handle()` 后调平台 API；OpenMP 用环境变量给运行时统一的栈大小。 |
| 线程属性与亲和：CPU 亲和 [3.6 节](ch03.md)、[22.5 节](ch22.md) | `无` | `pthread_setaffinity_np` | `无` | `OMP_PROC_BIND`、`OMP_PLACES` | `--bind-to`、`--map-by` | `sched_setaffinity`、`sched_getaffinity` | 亲和是提示，受 cgroup 和调度器限制；OpenMP 用环境变量，MPI 用启动器选项，标准库都没有。 |
| 进程创建与等待：fork [25.1 节](ch25.md) | `无` | `fork` | `无` | `无` | `无` | `clone`、`clone3` | `fork` 复制整个进程，多线程进程里只有调用线程活下来；`clone` 用标志选择共享哪些资源，`clone3` 是可扩展版本。 |
| 进程创建与等待：exec 与 spawn [25.2 节](ch25.md) | `无` | `execve`、`execl`、`execvp`、`posix_spawn` | `无` | `无` | `无` | `—` | `exec` 替换映像，成功不返回；多线程进程里 `posix_spawn` 比手写 `fork`+`exec` 安全。 |
| 进程创建与等待：回收与监督 [25.3 节](ch25.md)、[25.6 节](ch25.md) | `无` | `wait`、`waitpid`、`SIGCHLD` | `无` | `无` | `无` | `pidfd_open`、`waitid(P_PIDFD)`、`pidfd_send_signal`、`close_range`、`prctl(PR_SET_CHILD_SUBREAPER)` | 子进程退出后不回收会留下僵尸，标准信号会合并，`SIGCHLD` 处理要循环 `waitpid`；pidfd 绑定进程，不怕 PID 复用。 |
| 信号：安装处置 [26.1 节](ch26.md)、[26.2 节](ch26.md) | `无` | `sigaction`、`sigemptyset`、`sigaddset` | `signal`、`raise` | `无` | `无` | `—` | `sigaction` 能控制 `sa_mask`、`SA_RESTART`、`SA_SIGINFO`；`signal` 的语义不完整，新代码不该用它。 |
| 信号：屏蔽与同步等待 [26.4 节](ch26.md) | `无` | `pthread_sigmask`、`sigprocmask`、`sigwait`、`sigwaitinfo`、`sigtimedwait` | `无` | `无` | `无` | `signalfd` | 多线程里用 `pthread_sigmask` 改当前线程掩码；要 `sigwait` 或 `signalfd`，目标信号必须在所有线程屏蔽。 |
| 信号：定向发送 [26.4 节](ch26.md) | `无` | `pthread_kill`、`kill`、`raise` | `raise` | `无` | `无` | `pidfd_send_signal` | `pthread_kill` 定向到线程，`kill` 定向到进程；`pthread_t` 会复用，`sig=0` 不能证明线程还活着。 |
| 定时器：信号通知 [33.4 节](ch33.md) | `chrono` | `timer_create` | `无` | `无` | `无` | `timer_create`、`timerfd_create` | `chrono` 只是时间点和时长，不负责触发；`timer_create` 到期发信号或通知线程，适合低频定时。 |
| 定时器：fd 通知 [33.3 节](ch33.md)、[33.4 节](ch33.md) | `无` | `timerfd_create`、`timerfd_settime` | `无` | `无` | `无` | `timerfd_create`、`timerfd_settime`、`epoll` | `timerfd` 把到期变成可读事件，`read` 返回到期次数；绝对定时用 `CLOCK_MONOTONIC` 才不随校时漂移。 |
| I/O 复用与唤醒：select 与 poll [32.3 节](ch32.md) | `无` | `select`、`poll` | `无` | `无` | `无` | `select`、`poll` | `select` 受 `FD_SETSIZE` 限制且每次重建集合；`poll` 用数组，没有 1024 上限，但仍要每次传完整数组。 |
| I/O 复用与唤醒：epoll [32.4 节](ch32.md) | `无` | `无` | `无` | `无` | `无` | `epoll_create1`、`epoll_ctl`、`epoll_wait`、`epoll_pwait` | epoll 把兴趣表放在内核，`epoll_wait` 只返回就绪事件；ET 必须读到 `EAGAIN`，`EPOLLONESHOT` 用完要 `MOD` 重置。 |
| I/O 复用与唤醒：eventfd、signalfd、timerfd [26.3 节](ch26.md)、[33.3 节](ch33.md) | `无` | `pipe`、`pipe2` | `无` | `无` | `无` | `eventfd`、`signalfd`、`timerfd` | 统一事件源把信号、定时、线程唤醒都变成 fd；`eventfd` 是计数，`signalfd` 要先屏蔽信号，`timerfd` 读的是到期次数。 |
| I/O 复用与唤醒：io_uring [36.1 节](ch36.md)、[36.2 节](ch36.md) | `无` | `无` | `无` | `无` | `无` | `io_uring_setup`、`io_uring_enter` | io_uring 用 SQ/CQ 提交和收割，liburing 的 `io_uring_queue_init`/`io_uring_submit`/`io_uring_wait_cqe` 只是包装；完成队列不是 future，缓冲区要活到 CQE 回来，CQE 要 `io_uring_cqe_seen` 归还。 |
| IPC：管道与 socketpair [27.1 节](ch27.md) | `无` | `pipe`、`pipe2`、`socketpair` | `无` | `无` | `无` | `pipe2`、`socketpair` | 管道是半双工字节流，`PIPE_BUF` 以内的一次写才原子；`socketpair` 是全双工 UNIX socket，`SOCK_SEQPACKET` 保留边界。 |
| IPC：共享内存 [27.2 节](ch27.md) | `无` | `shm_open`、`ftruncate`、`mmap`、`shm_unlink` | `无` | `无` | `无` | `memfd_create`、`shmget`、`shmat`、`shmdt`、`shmctl` | POSIX 共享内存是有名字的 fd 对象，`shm_unlink` 只删名字；System V 用 key 和 `IPC_RMID`，`memfd_create` 创建匿名 fd。 |
| IPC：进程间同步 [27.3 节](ch27.md) | `无` | `sem_open`、`sem_wait`、`sem_post`、`sem_unlink`、`pthread_mutexattr_setpshared`、`pthread_mutex_consistent` | `无` | `无` | `无` | `semget`、`semop`、`semctl`、`futex` | 跨进程的 `sem_t` 和互斥量必须放在共享内存里；`SEM_UNDO` 只补偿计数，不是崩溃恢复，robust mutex 才能处理持有者死亡。 |
| IPC：消息队列 [27.4 节](ch27.md) | `无` | `mq_open`、`mq_send`、`mq_receive`、`mq_notify` | `无` | `无` | `MPI_Send`、`MPI_Recv` | `msgget`、`msgsnd`、`msgrcv`、`msgctl` | POSIX 消息队列有边界和优先级，但不是 fd；MPI 消息带 communicator/tag，走进程间网络，不经过内核消息队列。 |
| IPC：传递文件描述符 [27.5 节](ch27.md) | `无` | `sendmsg`、`recvmsg`、`SCM_RIGHTS` | `无` | `无` | `无` | `pidfd_getfd`、`MSG_CMSG_CLOEXEC` | `SCM_RIGHTS` 只能走 AF_UNIX，接收方拿到同一打开文件描述的副本；`pidfd_getfd` 跨进程复制 fd，要 ptrace 权限。 |
| 编译与链接：线程运行时 [3.5 节](ch03.md)、[22.1 节](ch22.md)、[29.1 节](ch29.md) | `-pthread` | `-pthread` | `-pthread` | `-fopenmp` | `mpicc`、`mpirun` | `-pthread` | `-pthread` 同时作用于预处理和链接，glibc 2.34 起 libpthread 并入 libc；MPI 程序必须用 `mpicc`/`mpirun`，否则头文件和库路径对不上。 |
| 编译与链接：可选后端 [23.1 节](ch23.md)、[34.3 节](ch34.md)、[36.2 节](ch36.md) | `-ltbb`、`-latomic` | `-lrt`（已不需要） | `无` | `无` | `无` | `-levent`、`-levent_pthreads`、`-luring` | `-ltbb` 是 libstdc++ 并行算法的后端，`-latomic` 给 16 字节原子；`-lrt` 在新 glibc 上已不需要，事件库和 io_uring 各有自己的库。 |

## 最容易混的三组

- `sem_wait` 与 `acquire` 都没有持有者：任何线程都能 `post`/`release`，接口不检查“谁拿的”。
- `pthread_cancel` 是延迟取消，只在取消点生效；`request_stop` 是协作停止，目标不检查就不会停。
- `MPI_Barrier` 是进程级，所有 rank 都要到；`std::barrier` 是线程级，参与线程数在构造时固定。

## 延伸阅读

- [C++ 并发设施索引](https://en.cppreference.com/w/cpp/thread)：按 `thread`、`mutex`、`condition_variable`、`future`、`atomic` 分组查标准接口。
- [pthreads(7)](https://man7.org/linux/man-pages/man7/pthreads.7.html)：POSIX 线程接口、取消点和线程属性。
- [OpenMP 5.2 规范](https://www.openmp.org/specifications/)：查 `parallel`、`task`、`cancel`、`atomic` 和 `flush` 的准确定义。
- [MPI 4.1 规范](https://www.mpi-forum.org/docs/)：查集合通信、非阻塞通信和 `MPI_Cancel` 的语义。
- [Linux 系统调用索引](https://man7.org/linux/man-pages/man2/)：按名字查 `clone`、`futex`、`eventfd`、`pidfd_*` 等系统调用。
