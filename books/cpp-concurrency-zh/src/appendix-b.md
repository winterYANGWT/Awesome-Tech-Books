# 附录 B 旧写法迁移表

先按旧写法查到行，再看“取代它的”和“起始版本”，最后点进“本书案例”读原因；表前的版本索引可以按标准或内核版本反查。

## 按版本筛选

| 版本节点 | 这一档能查到的迁移 | 代表条目 |
|---|---|---|
| C++11 | `std::thread`/`mutex`/`condition_variable`/`future`/`atomic` 成为基线；`call_once`、`make_exception_ptr` | `std::thread` 传参、`std::atomic`、`std::async` |
| C++17 | `scoped_lock`、`invoke_result_t`、`shared_mutex`、`reduce`/`scan`、`optional`、`noexcept` | `scoped_lock`、`reduce`/`scan` |
| C++20 | `jthread`/`stop_token`、`latch`/`barrier`/信号量、`atomic::wait`、`atomic_ref`、`atomic<shared_ptr>`、`unseq` | `jthread`、`atomic::wait` |
| C++23 | `move_only_function`、`expected`、移除标准 GC | `move_only_function` |
| C++26 | `std::execution`、`hazard_pointer`、`rcu`、`fetch_max`/`fetch_min`、弃用 `consume`（本机未实现） | `std::execution`、风险指针 |
| glibc 2.34 | libpthread 并入 libc，编译仍写 `-pthread` | `-lpthread` |
| Linux 2.6.22–5.11 | `signalfd`/`eventfd` 2.6.22、`timerfd` 2.6.25、`epoll_create1` 2.6.27、`accept4` 2.6.28、`SO_REUSEPORT` 3.9、`EPOLLEXCLUSIVE` 4.5、`io_uring` 5.1、`pidfd` 5.3/5.4、`epoll_pwait2` 5.11 | 进程/信号、I/O |
| OpenMP 4.0/5.1 | 4.0 `simd`/`target`/`depend`/`cancel`；4.5 `taskloop`；5.1 `masked`，`master` 弃用 | `master` → `masked` |
| MPI 3.0/4.0 | 3.0 非阻塞集合、移除 C++ 绑定；4.0 持久集合、大计数 | `MPI_Iallreduce`、`MPI_Allreduce_init` |
| Libevent 2.0 | `event_base_new()`、`evthread_use_pthreads()` | `event_init` |

## 迁移表

本表汇总第十部分各章的“旧资料提示”；各章只保留一句并链接到对应案例，不重复表的全文。

### 1. 线程与同步（27 行）

| 旧写法 | 典型出处/场景 | 取代它的 | 起始版本 | 本书案例 | 今天还用得着吗 | 备注 |
|---|---|---|---|---|---|---|
| `-lpthread` | 早期 Pthreads 教程、服务器构建脚本、POSIX 现状综述 | `-pthread` | glibc 2.34（libpthread 并入 libc） | [37.1 节](ch37.md) | `-lpthread` 今天仍能链接，但只影响链接；新代码写 `-pthread`，老 glibc 也能用 | `-pthread` 同时定义 `_REENTRANT` 并参与链接，`-lpthread` 只链接 |
| `long`/`int` 与 `void*` 互转 | Pthreads 传参和返回值示例 | `intptr_t`；C++ 用 `std::thread` 变参模板 | C++11；POSIX 提供 `intptr_t` | [37.1 节](ch37.md) | 维护 C 接口仍要用 `intptr_t`；C++ 新代码不再打包 `void*` | 指针宽度在 LP64/LLP64 上不同，直接强转是实现定义 |
| `foo(get_num(), get_num())` 依赖求值顺序 | C++11 线程构造函数示例、旧并发教材 | 把有副作用的调用拆成两条语句 | C++17 起实参求值 indeterminately sequenced | [37.1 节](ch37.md) | 旧代码不必改，但不能依赖顺序；新代码不要在同一实参表里放两个有副作用的调用 | 顺序仍未指定，C++17 只排除交错和未定义行为 |
| `thread_guard`/`joining_thread`/`join_threads` 手写 RAII | C++11 线程守卫、并行任务队列 | `std::jthread`；线程数组用 `std::vector<std::jthread>` | C++20 | [37.2 节](ch37.md) | 包装外部传入的 `std::thread` 仍要守卫；自己创建的线程用 `jthread` | 析构先 `request_stop()` 再 `join()` |
| `pthread_cancel` 从外部结束线程 | Pthreads 服务器、线程池的关闭逻辑 | `std::jthread` + `std::stop_token`/`stop_source`/`stop_callback` | C++20 | [37.2 节](ch37.md) | 线程阻塞在不看 token 的 `read` 上时，仍可能要取消或关闭 fd | `pthread_cancel` 是延迟取消，只在取消点生效 |
| `thread_local` 标志 + `interrupt()`/`interruption_point()` 自造中断 | C++11 协作中断示例 | `stop_token`；`condition_variable_any::wait(lock, token, pred)` | C++20 | [37.2 节](ch37.md) | 旧封装可保留，但新代码用标准停止令牌 | `condition_variable` 没有 token 重载，要在 `stop_callback` 里 `notify_all()` |
| `sleep`/`usleep` 当线程间同步 | Pthreads 生产者-消费者示例、旧服务器代码 | 条件变量 `wait(lock, pred)`；只等一个值用 `atomic::wait` | `usleep` 在 POSIX.1-2008 删除；`atomic::wait` 为 C++20 | [37.3 节](ch37.md) | 纯延时可 `nanosleep`/`clock_nanosleep(CLOCK_MONOTONIC)`；不能拿延时当同步 | 睡眠不建立 happens-before，共享读仍是数据竞争 |
| `std::lock(m1, m2)` + 两个 `adopt_lock` | C++11 转账示例、双锁代码 | `std::scoped_lock l(m1, m2)` | C++17 | [37.4 节](ch37.md) | 锁数量运行期才定（桶数组）时仍要 `std::lock` 加固定顺序 | `scoped_lock` 的参数个数编译期固定 |
| 手写 `hierarchical_mutex` 做层级检查 | C++11 死锁防护示例 | 标准仍无层级锁；靠固定顺序或 `scoped_lock` | —（标准未提供） | [37.4 节](ch37.md) | 大型代码库保留自建层级锁仍有价值；小范围用固定顺序即可 | 标准不做跨调用的顺序检查 |
| 假设 `pthread_mutex_lock` 按优先级/FIFO 排队 | Pthreads 服务器、旧同步教材 | 不依赖等待顺序；共享状态用条件变量或固定加锁顺序 | POSIX 不保证 | [37.4 节](ch37.md) | 旧代码不能靠公平性推正确性；要公平就自己排队 | `PTHREAD_MUTEX_ERRORCHECK`/`RECURSIVE` 不是公平开关 |
| `pthread_cond_signal` 在 `wait` 之前发，或 `if` 只查一次谓词 | Pthreads 条件变量示例 | 持锁改状态后 `signal`/`broadcast`，`while` 重查谓词 | POSIX；C++11 `wait(lock, pred)` | [37.4 节](ch37.md) | 旧代码里的丢失唤醒要补谓词循环；`atomic::wait` 也允许虚假唤醒 | 销毁仍有等待者的条件变量是未定义行为 |
| 手写计数器 + 条件变量做屏障，或认为 `pthread_barrier_*` 到处都有 | Pthreads 并行教材、跨平台服务器代码 | `std::barrier`/`std::latch`；C 用 `pthread_barrier_*` 或自带实现 | C++20；`pthread_barrier_*` 自 POSIX.1-2001 起为可选 | [37.5 节](ch37.md) | macOS 仍无 `pthread_barrier_*`，跨平台 C 代码要自带实现 | 手写版要复位计数并区分轮次，否则第二轮死锁 |
| Concurrency TS 的 `flex_barrier`/`latch`/`barrier` | TS 同步设施示例、C++11 同步设施综述 | `std::barrier`/`std::latch`；无 `flex_barrier` | C++20 | [37.5 节](ch37.md) | TS 名字不再可用；完成函数不能改下一轮参与数 | `count_down_and_wait` 改名 `arrive_and_wait` |
| 用 `std::counting_semaphore` 做跨进程同步，或把 `sem_t` 放在普通全局变量里 | Pthreads 进程间同步、C++20 信号量示例 | 进程内用 C++20 信号量；跨进程仍用 POSIX `sem_open`/`sem_init` | C++20；POSIX.1-2001 | [37.5 节](ch37.md) | 跨进程必须 POSIX；`pshared=1` 要求 `sem_t` 本身在共享内存 | `std::counting_semaphore` 只有进程内语义，且没有持有者 |
| `pthread_attr_getstackaddr`/`setstackaddr`，或把 `pthread_attr_t` 展开成 `char[36]` | 32 位时代的 Pthreads 教程 | `pthread_attr_getstack`/`setstack`；大小用 `setstacksize` | POSIX.1-2008 删除旧接口 | [37.6 节](ch37.md) | 旧代码编译会警告；新代码用 `getstack`/`setstack`，并按不透明类型处理 | x86-64 glibc 2.43 的 `pthread_attr_t` 是 56 字节 |
| 按 32 位 4 GB 地址空间估算线程数上限 | 旧并发教材的线程数练习 | 按 `RLIMIT_AS`、`RLIMIT_NPROC`、cgroup `pids.max`、`threads-max` 实测 | —（64 位平台普及） | [37.6 节](ch37.md) | 64 位下地址空间通常不是瓶颈；容器里先撞 `pids.max` 或栈大小 | 默认 8 MiB 栈只是虚拟地址保留，不是立即占用 |
| 认为标准库能设线程栈、guard、亲和性 | C++ 线程封装、服务器线程池 | pthread 属性接口；标准库没有属性对象 | C++11 起标准库未提供 | [37.6 节](ch37.md) | 要设栈、guard、绑核仍用 pthread；`jthread` 没有栈大小接口 | `native_handle()` 在 Linux 上可拿到 pthread 对象 |
| `std::result_of<F(Args...)>::type` | C++11 任务队列、异步任务示例、类型推导综述 | `std::invoke_result_t<F, Args...>` | C++17 弃用，C++20 移除 | [37.7 节](ch37.md) | 维护旧代码会见到；新代码用 `invoke_result_t`/`decltype` | libstdc++ 15 仍保留旧名作兼容，不代表标准还有 |
| `std::function<void()>` 装 `packaged_task` 或 move-only lambda | C++11 任务队列、异步任务示例 | `std::move_only_function` | C++23 | [37.7 节](ch37.md) | C++20 及以前仍要手写包装或 `unique_ptr`；新代码可直接用 | `std::function` 要求目标可拷贝 |
| 手写 `function_wrapper` 做类型擦除 | C++11 任务队列、`packaged_task` 队列 | `std::move_only_function` | C++23 | [37.7 节](ch37.md) | 要兼容 C++17 的库仍保留；新代码不必再写虚函数包装 | 内部用 `unique_ptr` 持有目标 |
| `std::copy_exception` | 旧 C++11 异常传递代码 | `std::make_exception_ptr` | C++11（标准名自始就是 `make_exception_ptr`） | [39.4 节](ch39.md) | 旧名字要改；`copy_exception` 从来不是标准名 | 跨任务边界存进 shared state 或 `exception_ptr` |
| `packaged_task` 的分配器构造函数（`allocator_arg`） | C++11 任务队列、旧标准库用法 | 删除分配器参数，按值/移动构造 | C++17 移除 | [37.7 节](ch37.md) | 旧代码的分配器写法要删；C++26 另有调整，迁移时以目标标准为准 | C++26 的调整需核实 |
| 把 C11 `<threads.h>` 当统一线程接口 | C11 移植代码、跨平台 C 库 | Linux 上仍优先 pthread；C++ 用 `std::thread`/`jthread` | C11；glibc 2.28 起提供 | [37.8 节](ch37.md) | 可移植 C 代码可用；TSan 对 glibc C11 threads 支持不完整 | 可选特性，定义 `__STDC_NO_THREADS__` 就可以不提供 |
| `pthread_create` 只传一个 `void*`，参数打包成 `struct` | Pthreads 入门示例、C 服务器线程 | `std::thread`/`jthread` 的变参模板；共享对象用 `std::ref` | C++11/C++20 | [37.1 节](ch37.md) | 维护 C 代码仍要打包；C++ 新代码用标准库，注意对象寿命 | `std::ref` 只包地址，不延长寿命 |
| 只用 C++11/17 的线程设施讲并发 | C++11/17 并发书、旧项目 | C++20 `jthread`/`stop_token`/`latch`/`barrier`/`semaphore`/`atomic::wait` | C++20 | [37.2 节](ch37.md) | 受编译器限制的旧项目仍停在 C++17；新项目可用 C++20 | C++20 设施在 GCC 15/Clang 21 上均可编译运行 |
| 用 `promise`/`shared_future` 做起跑门 | C++11 并行示例、任务起跑同步 | `std::latch`/`std::barrier` | C++20 | [37.5 节](ch37.md) | 旧代码仍能跑；一次性起跑用 `latch` 更直接 | `shared_future` 的 `get()` 返回 `const T&` |
| 读写锁用 pthread `pthread_rwlock_*`，或把 `shared_timed_mutex` 当唯一选择 | C++14/17 读写锁示例、并发容器章节、性能章节 | `std::shared_mutex`（C++17）/`shared_timed_mutex`（C++14） | C++14/C++17；POSIX.1-2001 | [37.4 节](ch37.md) | 读多写少仍可用；标准不保证优先级，写者可能饿死 | `shared_lock` 读、`unique_lock` 写 |

### 2. 内存模型与原子（19 行）

| 旧写法 | 典型出处/场景 | 取代它的 | 起始版本 | 本书案例 | 今天还用得着吗 | 备注 |
|---|---|---|---|---|---|---|
| `volatile` 忙等标志 | 并行教材的忙等示例、C++ 并发书的内存模型章节、并行算法教材、2011 年前后的性能章节 | `std::atomic<T>` + release/acquire；阻塞等待用 `atomic::wait`/`notify_one` | C++11 / C++20 | [38.1 节](ch38.md)、[38.5 节](ch38.md) | MMIO 和 `volatile sig_atomic_t` 仍用 `volatile`；线程间同步不要用 | `volatile` 不保证原子性、不建立 happens-before |
| 双重检查锁（DCLP） | 旧 C++ 单例、懒初始化代码 | 局部 `static` 初始化 / `std::call_once` + `once_flag` | C++11 | [38.2 节](ch38.md) | 旧代码维护时优先改；`atomic<shared_ptr>` 可原子换指针但本机非无锁 | `call_once` 的函数抛异常时 flag 不置位 |
| `ATOMIC_VAR_INIT` / `atomic_init` | C++11 原子初始化、旧并发教材 | 默认构造 / 显式初值；静态存储期用常量初始化 | C++20 弃用 | [38.3 节](ch38.md) | 旧代码仍能编译；新代码不需要 | `atomic_init` 不能给已构造的对象重新赋值 |
| `ATOMIC_FLAG_INIT` 是 `atomic_flag` 唯一可靠初始化 | C++11 原子标志示例 | 默认构造即 clear；`test()`、`wait`/`notify` | C++20 | [38.3 节](ch38.md)、[38.4 节](ch38.md) | 旧代码仍可用（LWG3659 未弃用）；新代码默认构造 | C++20 起 `atomic_flag` 不再是唯一保证无锁的类型 |
| 块作用域 `std::atomic<int> a;` 值不确定 | C++11/17 原子示例 | 默认构造值初始化 `T()`，或显式 `{0}` | C++20 | [38.3 节](ch38.md) | C++17 及以前必须显式初始化；C++20 起默认是 0 | 静态存储期本来就零初始化 |
| `atomic_flag` 是唯一保证无锁的原子类型 | C++11 原子教材 | C++20 `atomic_signed_lock_free`/`atomic_unsigned_lock_free`；C++17 `is_always_lock_free` | C++17 / C++20 | [38.4 节](ch38.md) | 任意 `atomic<T>` 仍要查 `is_lock_free()`；16 字节尤其要看平台 | `ATOMIC_*_LOCK_FREE` 宏仍在 |
| 16 字节 `atomic<struct>` 一定无锁 | 无锁队列、`counted_node_ptr` 示例 | 先查 `is_lock_free()`；必要时换平台/编译选项 | 平台和编译选项相关 | [38.4 节](ch38.md) | 要 16 字节原子时先在本机测；GCC 15 加 `-mcx16` 仍可能 false | Clang 21 加 `-mcx16` 可为 true；可能要 `-latomic` |
| 浮点 `atomic` 只能 load/store/CAS | C++11 原子教材、浮点累加示例 | `fetch_add`/`fetch_sub` | C++20 | [38.4 节](ch38.md) | 旧 CAS 循环仍正确；新代码可直接 `fetch_add` | `compare_exchange` 按位比较，`+0.0`/`-0.0` 会被判不等 |
| 整型原子没有 `fetch_max`/`fetch_min`，只能加锁 | 无锁算法、求最大值示例 | CAS 循环；C++26 `fetch_max`/`fetch_min` | C++26（本机未实现） | [38.4 节](ch38.md) | 今天仍要 CAS 循环；C++26 实现后再换 | 除法、乘法、移位仍没有原子指令 |
| 非成员 `atomic_load(&sp)`/`atomic_store(&sp, p)` | C++11 原子 `shared_ptr` 示例 | `std::atomic<std::shared_ptr<T>>` | C++20 | [38.6 节](ch38.md) | 旧写法 C++20 起弃用；新代码用成员版 | 标准不保证无锁，本机 libstdc++ 15 `is_lock_free()==false`；C++26 草案已移除非成员重载 |
| Concurrency TS 的 `std::experimental::atomic_shared_ptr<T>` | TS 原子智能指针示例 | `std::atomic<std::shared_ptr<T>>`/`atomic<weak_ptr<T>>` | C++20 | [38.6 节](ch38.md) | TS 类型不要再用；新代码用标准成员版 | TS 未进标准；不要混用 experimental 命名空间 |
| `memory_order_consume` + `std::kill_dependency` | 内存序章节、标准版本综述 | `memory_order_acquire`/`release`；需要额外排序用 fence | C++26 弃用 consume（P3475R2） | [38.7 节](ch38.md) | 旧代码仍编译，语义等同 `acquire`；新代码不要写 | GCC/Clang 一直把 consume 当 acquire |
| 想给普通对象做原子访问，却只靠普通变量 + 栅栏 | C++11 无锁示例 | `std::atomic_ref<T>`（C++20） | C++20 | [38.4 节](ch38.md) | 普通数组只想原子访问几个元素时合适；要求对齐且所有访问都走它 | `atomic_ref` 不替代栅栏，也不允许绕过它直接读写 |
| 标准 GC 接口（`std::pointer_safety` 等） | C++11 无锁章节、回收方案综述 | `<hazard_pointer>`/`<rcu>`；或 `shared_ptr` + 锁 | C++23 移除 GC；C++26 加风险指针/RCU | [38.8 节](ch38.md) | 旧代码 C++23 起编译不过；新代码先 `shared_ptr` + 锁，确认瓶颈再上 | 本机 GCC 15/Clang 21 未实现风险指针/RCU |
| 把风险指针/RCU 当“将来标准会加” | C++11 无锁回收综述 | C++26 `std::hazard_pointer`/`std::rcu_obj_base` | C++26 | [38.8 节](ch38.md) | 标准已有、本机未实现；今天仍要自己实现或用第三方 | `hazard_pointer_obj_base`、`retire()`、`rcu_synchronize()` |
| 无锁结构不回收节点，靠泄露或“程序退出再释放” | 无锁栈/队列示例 | 风险指针/RCU；低竞争用 `shared_ptr` + 锁 | C++26（标准设施） | [38.8 节](ch38.md) | 长跑服务不能靠泄露；先测瓶颈再上无锁回收 | `atomic<shared_ptr>` 本机非无锁，不能替代 |
| 在标准库里找无锁容器 | C++11 无锁章节、并发容器综述 | 到 C++26 仍无标准无锁容器；用 Boost.Lockfree 或自己实现 | C++26 仍未标准化 | 附录 B；另见 [38.4 节](ch38.md)、[38.8 节](ch38.md) | 需要时用第三方并测；标准容器加锁仍是默认 | 标准容器不保证无锁 |
| 把缓存行写死为 64 字节 | 伪共享示例、无锁性能章节、并行性能章节 | `std::hardware_destructive_interference_size` 或目标平台实测 | C++17 | [40.7 节](ch40.md) | x86-64 常见 64 字节；ARM 可能是 64 或 128 | 该常量是实现定义的提示值，本机 GCC 15/Clang 21 都是 64 |
| 只靠“枚举所有线程交织”的算法检查器 | 无锁算法验证、并发测试工具综述 | ThreadSanitizer（`-fsanitize=thread`）+ 压力测试 + 模型检查器 | 工具现状 | 附录 B；另见 [38.8 节](ch38.md) | TSan 只覆盖实际执行到的路径；不能证明 relaxed 正确 | CDSChecker/GenMC 本机不可用，未验证 |

### 3. 异步任务（15 行）

| 旧写法 | 典型出处/场景 | 取代它的 | 起始版本 | 本书案例 | 今天还用得着吗 | 备注 |
|---|---|---|---|---|---|---|
| `std::async` 不传策略就当后台线程 | C++11 异步任务示例 | 显式 `launch::async`/`launch::deferred`；可控并发自己建池 | C++11 | [39.1 节](ch39.md) | 单个任务仍可用；批量、限流、取消不要用 | 不传策略等价于 `async` 与 `deferred` 的合并，可能根本不执行 |
| `std::async` 保证用满硬件、不会超额申请 | 并行算法章节、异步任务章节 | 标准没有这个保证；自己建池或用第三方线程池 | C++11 起就没有保证 | [39.1 节](ch39.md)、[40.5 节](ch40.md) | 旧结论要删；需要线程数上限就自己建池 | `launch::async` 的 future 析构会阻塞到任务完成 |
| `future` 析构当取消 | C++11 异步代码 | 显式 `get()`/`wait()`；取消改 `stop_token` | C++11 | [39.1 节](ch39.md) | `launch::async` 析构是等待，`launch::deferred` 析构是丢弃任务 | `std::future` 没有取消接口 |
| `future.then(...)` | Concurrency TS 示例、异步链 | C++26 `std::execution` 的 `then`；今天用 `async`+`get` 或第三方 | TS 未并入；C++26（本机未实现） | [39.2 节](ch39.md) | `std::future` 至今没有 `then`；旧代码编译不过 | libstdc++ 15 没有 `<experimental/future>` |
| TS 的 `when_all`/`when_any` 和 `std::experimental::future` | Concurrency TS 示例、任务组合 | C++26 `std::execution::when_all`；`when_any` 不在 N5050 | C++26（本机未实现） | [39.2 节](ch39.md)、[39.6 节](ch39.md) | 今天手写多次 `get` 或第三方；`when_any` 仍无标准接口 | TS 未并入 C++17/20/23 |
| `high_resolution_clock` / `clock()` 计时 | 旧 chrono 示例、并行计时章节 | `steady_clock`（测间隔）/`system_clock`（日历） | C++11 | [39.3 节](ch39.md) | libstdc++ 15 的 `high_resolution_clock` 是 `system_clock` 别名、`is_steady=false` | `clock()` 测的是 CPU 时间，不能当墙钟 |
| `wait_until(lock, duration)`；`cv_status::time_out` | 旧表/旧教程的超时示例 | `wait_until(time_point)`；`cv_status::timeout` | C++11 | [39.3 节](ch39.md) | 旧代码编译错或语义错；`_for` 收 duration，`_until` 收 time_point | `time_out` 是旧表笔误 |
| `chrono` 只有时钟和时长 | C++11 时间库示例 | C++20 日历、时区、`utc_clock`/`tai_clock`/`gps_clock` | C++20 | [39.3 节](ch39.md) | 旧代码不必改；跨时区调度用新设施 | 计时和日历不要混用时钟 |
| `shared_future` 当一次结果/拷贝 | C++11 多消费者示例 | `shared_future<T>::get()` 返回 `const T&`，副本共享状态 | C++11 | [39.3 节](ch39.md) | 多消费者仍适用；不需要改 | 每个 `share()` 副本都能 `get` |
| 动态异常规格 `throw(int)`/`throw()` | C++98/11 代码、异常跨任务边界 | `noexcept` | C++17 移除动态异常规格 | [39.4 节](ch39.md) | 空 `throw()` 仍被接受但应写 `noexcept` | C++17 起 `throw(T)` 直接编译失败 |
| 异常不捕获就让它离开线程函数 | C++11 任务队列、线程池 | `packaged_task`/`promise::set_exception`；`make_exception_ptr` | C++11 | [39.4 节](ch39.md) | 工作线程异常必须捕获再存 shared state；否则 `std::terminate` | 带执行策略的算法也在元素函数抛异常时 terminate |
| 线程池空队列 `try_pop` 失败就 `yield` | C++11 线程池、任务队列 | 条件变量 / `counting_semaphore` / `atomic::wait` | C++20（信号量/原子等待） | [39.5 节](ch39.md) | 旧池子维护时改；关闭顺序要 `done` + `notify_all` + join | `yield` 只让出时间片，不阻塞 |
| 在标准库里找 `std::thread_pool`/工作窃取 | C++11/23 并发书、线程池章节 | 到 C++23 仍无 `std::thread_pool`；C++26 `std::execution` 不是现成池 | C++23/C++26 | [39.5 节](ch39.md) | 仍要自己建池或用第三方；`execution` 本机未实现 | 标准没有可配置线程数的池类 |
| 把 Executors 提案当马上可用的 `std::executor` | C++17/20 异步代码、executors 综述 | C++26 sender/receiver；今天用线程池/Asio/协程 | C++26（本机未实现） | [39.6 节](ch39.md) | 旧代码等不到 `std::executor`；先学任务图与执行位置分离 | `std::execution` 不是 `std::future` 的 drop-in |
| 用回调串联异步步骤，或以为 `std::future` 可 `co_await` | C++11/14 异步代码、C++20 协程示例 | 自己写 awaiter 或用 Asio/cppcoro/libunifex；C++26 sender/receiver | C++20/C++26 | [39.7 节](ch39.md) | I/O 密集可上协程；CPU 密集仍交线程池 | 标准库没有现成异步 task 类型 |

### 4. 并行模型（17 行）

| 旧写法 | 典型出处/场景 | 取代它的 | 起始版本 | 本书案例 | 今天还用得着吗 | 备注 |
|---|---|---|---|---|---|---|
| 把 OpenMP 等同于 `parallel for` + `reduction` + `critical` | OpenMP 3.0 教材、并行模型综述 | 3.0 `task`/`taskwait`；4.0 `simd`/`target`/`depend`/`cancel`；4.5 `taskloop`；5.x `masked` 等 | OpenMP 3.0–5.2 | [40.1 节](ch40.md) | 规则循环仍最适合 `parallel for`；递归/不规则用 `task` | GCC 15 的 `_OPENMP=201511`（4.5）；Clang 21 libomp 报 5.1 |
| `#pragma omp master` 表示线程 0 执行 | OpenMP 3.0/4.0 教材 | `masked`（默认 `filter(0)`），或 `single` | OpenMP 5.1 起 `master` 弃用 | [40.2 节](ch40.md) | GCC 15 仍接受 `master`；可移植性按目标编译器实测 | `master`/`masked` 无隐式屏障，`single` 有 |
| 给 `parallel` 或合并的 `parallel for` 加 `nowait` | 旧 OpenMP 教程 | 只用于 `for`/`sections`/`single` 等 worksharing | OpenMP 规范 | [40.2 节](ch40.md) | 旧代码编译失败；`single nowait` 合法 | OpenMP 5.2 还允许 `target`/`taskwait` |
| `schedule(static, n/thread_count)` 当块划分；`guided` 的 chunk 当固定块 | OpenMP 3.0/4.0 教材 | 连续块用 `schedule(static)`；要固定块才带 chunk；`guided` 的 chunk 是下限 | OpenMP 规范 | [40.2 节](ch40.md) | 旧代码结果依赖取整；`n/p` 可能算出 0 而被拒绝 | `dynamic` 省略 chunk 默认 1 |
| OpenMP `atomic` 只支持 `x op= expr` 和自增自减 | OpenMP 3.0 教材、原子形式综述 | OpenMP 5.0 起可带 memory-order；后续接受 `x = x op expr` 等形式 | OpenMP 5.0+ | [40.1 节](ch40.md) | 旧写法仍合法；`x += y++` 仍只保护 `x` | 具体形式版本需核实 |
| `parallel for` 每次执行都创建/合并线程 | 2011 年前后的 OpenMP 性能表 | 现代运行时通常复用线程池；结构上仍建议减少 fork-join | OpenMP 实现相关 | [40.7 节](ch40.md) | 17%–22% 的具体数字不能照搬；复用团队的结论仍成立 | 开销要在目标机器重测 |
| 加 `std::execution::par` 就一定并行、提速 | C++17 并行算法示例、并行模型综述 | 链接 TBB 并确认后端；`par` 只是许可 | C++17；libstdc++ 的 PSTL/TBB 后端 | [40.3 节](ch40.md) | 没有 TBB 头就串行；性能以实测线程数和耗时为准 | `-D_GLIBCXX_USE_TBB_PAR_BACKEND=0` 可模拟串行 |
| 策略版算法（含 `seq`）的异常会传播给调用者 | C++17 并行算法教程 | 四种策略都在元素函数抛异常时 `terminate`；要传播用不带策略的重载 | C++17 | [40.3 节](ch40.md) | 旧说法错；元素函数内部捕获再重抛 | 算法自己申请内存失败仍可能抛 `bad_alloc` |
| 标准执行策略只有 `seq`/`par`/`par_unseq` | C++17 并行教材 | C++20 增加 `unseq`；ranges 算法没有策略重载 | C++20 | [40.3 节](ch40.md) | 旧代码仍可用；向量化用 `unseq`，但不能有锁和阻塞 | C++26 草案给 ranges 加策略重载，本机未实现 |
| 给 `std::accumulate`/`partial_sum` 加 `par`，或手写分块前缀和 | C++11 并行示例、并行算法教材 | `std::reduce`/`transform_reduce`；`inclusive_scan`/`exclusive_scan` | C++17 | [40.4 节](ch40.md) | 要严格顺序仍用 `accumulate`/`partial_sum`；浮点末位可能不同 | 标准不保证常数和遍历次数 |
| `hardware_concurrency()` 当核数/线程数 | C++11 并发代码、并行度章节 | 结合 `sched_getaffinity`、cgroup `cpu.max` 和实测 | C++11 起接口未变 | [40.5 节](ch40.md) | 仍可作起点；可能返回 0，不区分 P/E 核、SMT、NUMA | 容器里常远大于可用 CPU |
| 一核最多一个线程，线程数按核数写死 | 2011 年前后的并行教材、SMT 前的性能章节 | 按 SMT/NUMA/配额实测；动态任务和线程池 | 硬件现状 | [40.5 节](ch40.md)、[40.7 节](ch40.md) | 教学假设仍可简化；测量要绑核、隔离 CPU、固定频率 | 两个逻辑线程共享一个核的执行单元 |
| MPI-1/2 的阻塞集合够用；小消息 `MPI_Send` 不会阻塞 | MPI-1/2 教材、并行模型综述 | `MPI_Iallreduce`/`Iallgather`/`Ibcast`；`MPI_Isend`+`Waitall`；MPI-4 持久集合 | MPI-3.0/4.0 | [40.6 节](ch40.md) | 阻塞集合仍适合小规模；重叠收益要测 | `MPI_Send` 可能缓冲后返回，也可能阻塞到匹配接收 |
| 派生数据类型一定比基本类型慢 | MPI 教材、粒子通信示例 | `MPI_Type_contiguous` 通常高效；带间隙的 `create_struct` 测过再决定 | MPI 实现相关 | [40.6 节](ch40.md) | 不要一概而论；按消息形状和实现实测 | 连续类型不必逐元素打包 |
| C++ 程序直接用 MPI 的 C++ 绑定 | MPI-2 教材、旧代码 | C API 或 Boost.MPI 等包装库 | MPI-3.0 移除 C++ 绑定 | [40.6 节](ch40.md) | 旧代码要改；本机 Open MPI 5.0.10 声明 MPI 3.1 | `MPI_Get_version` 是声明，接口是否存在要查头文件和符号 |
| 拿 2011 年测出的秒数、加速比、Amdahl 参数和忙等/互斥量/读写锁的性能排序直接外推 | 2011 年前后的并行教材、性能章节 | 目标机器重测；保留负载均衡、伪共享、通信开销等定性结论 | 硬件代际 | [40.7 节](ch40.md) | 秒数/倍数不可复用；Amdahl 和强/弱可扩展公式仍成立 | 今天还要看 roofline、NUMA、SMT 和功耗墙 |
| 认为 C++ 标准提供/应该提供 IPC | C++ 并发书、并行模型综述 | 到 C++23 仍无标准 IPC；用 MPI、socket、共享内存或 ZeroMQ | C++23 | [40.6 节](ch40.md) | 跨进程仍用系统接口或 MPI；C++26 是否有进展需核实 | 标准库只有线程，不含进程间通信 |

### 5. 进程与信号（18 行）

| 旧写法 | 典型出处/场景 | 取代它的 | 起始版本 | 本书案例 | 今天还用得着吗 | 备注 |
|---|---|---|---|---|---|---|
| `signal(sig, handler)` 装处理函数 | 旧 C 代码、服务器信号处理 | `sigaction` + `SA_SIGINFO`/`SA_RESTART` | POSIX.1 | [41.1 节](ch41.md) | 维护旧代码先确认 BSD/SysV 语义；新代码用 `sigaction` | `siginterrupt` 更老，只改重启行为 |
| 把“可重入”当“线程安全” | 旧服务器代码、POSIX/C11 综述 | 区分两者；处理函数只用 async-signal-safe 函数 | POSIX/C11 | [41.1 节](ch41.md) | `malloc` 线程安全但非 async-signal-safe；`_r` 版本仍首选 | `volatile sig_atomic_t` 可做信号标志，不能类推到线程 |
| 自管道把信号接进 `epoll` | 旧服务器、统一事件源示例 | `signalfd`/`eventfd` | Linux 2.6.22 | [41.2 节](ch41.md) | 跨平台、第三方库不能改屏蔽字时仍用自管道 | 处理函数只 `write`；管道要 `O_NONBLOCK` |
| SIGCHLD 默认忽略，或处理函数里只 `waitpid` 一次 | 旧服务器、子进程回收代码 | 循环 `waitpid(WNOHANG)`；`signalfd`；`pidfd` | Linux 2.6.22/5.3/5.4 | [41.2 节](ch41.md) | SIGCHLD 会合并；仍要循环回收 | `SIG_IGN`/`SA_NOCLDWAIT` 才自动回收 |
| 用 PID + `kill`/`waitpid` 等子进程 | 旧服务器、进程管理代码 | `pidfd_open` + `poll` + `waitid(P_PIDFD)`；`pidfd_send_signal` | Linux 5.3/5.4（send 5.1） | [41.2 节](ch41.md) | PID 复用问题仍要 pidfd；普通回收仍要 `waitpid` | pidfd 不负责回收 |
| `alarm`/`setitimer` + `SIGALRM` 做定时；升序链表/时间轮/时间堆自己维护 | 旧服务器、定时器章节 | `timerfd_create(CLOCK_MONOTONIC, ...)`；`timer_create` 或事件库定时器 | Linux 2.6.25；`setitimer` 在 POSIX.1-2008 obsolescent | [41.3 节](ch41.md) | 小工具粗超时 `alarm` 仍可；服务器优先 `timerfd` | 定时器容器跨平台时仍要自己写，单机 Linux 交给内核 |
| `SO_KEEPALIVE` 很快发现死连接 | 旧 TCP 代码、连接管理 | `TCP_USER_TIMEOUT`（2.6.37）+ 应用层心跳 | Linux 2.6.37 | [41.3 节](ch41.md)、[42.8 节](ch42.md) | keepalive 默认 7200 秒；仍可作最后一道 | `TCP_USER_TIMEOUT` 管已建立连接，不是 SYN 超时 |
| 多线程进程 `fork` + `pthread_atfork`；`fork`+`dup`+`execl` 起 CGI | 旧服务器、CGI 代码 | `posix_spawn`；或 fork 后立刻 exec；`clone3`/pidfd | POSIX.1-2001；Linux 5.3 `clone3`/`pidfd_open` | [41.4 节](ch41.md) | 单线程 prefork 仍可 `fork`；多线程优先 `posix_spawn` | fork 后、exec 前只能调 async-signal-safe 函数 |
| `open` 后补 `fcntl(F_SETFD, FD_CLOEXEC)` | 旧服务器、fd 管理代码 | `O_CLOEXEC`；socket 用 `SOCK_CLOEXEC` | Linux 2.6.23（`O_CLOEXEC`/`MSG_CMSG_CLOEXEC`） | [41.5 节](ch41.md) | 旧代码有 fork/exec 窗口；新代码创建 fd 时一次设好 | `FD_CLOEXEC` 只影响 exec，不影响 fork；批量设置可用 `close_range`（5.9，`CLOSE_RANGE_CLOEXEC` 5.11） |
| `pipe` 后再两次 `fcntl` | 旧服务器、自管道 | `pipe2` 同时带 `O_NONBLOCK` 和 `O_CLOEXEC` | Linux 2.6.27 | [41.2 节](ch41.md)、[41.5 节](ch41.md) | 旧代码可改；自管道仍常用 | 管道读端要非阻塞，写端要防满管道阻塞 |
| `accept` 后再 `fcntl` 设非阻塞/CLOEXEC | 旧服务器、连接接受代码 | `accept4` 同时带 `SOCK_NONBLOCK` 和 `SOCK_CLOEXEC` | Linux 2.6.28 | [41.5 节](ch41.md)、[42.3 节](ch42.md) | 老内核回退 `accept`+`fcntl`；新代码用 `accept4` | `accept4` 失败不会留下已接受的 fd |
| `SCM_RIGHTS` 收 fd 后不设 CLOEXEC | 旧多进程服务器、fd 传递 | `MSG_CMSG_CLOEXEC`；`SCM_RIGHTS` 仍是标准传递方式 | Linux 2.6.23 | [41.5 节](ch41.md) | 跨进程传 fd 仍用 `SCM_RIGHTS`；接收端加标志 | 一次最多 253 个 fd（`SCM_MAX_FD`，2.6.38 起） |
| System V `shmget`/`semget`/`msgget` | 旧服务器、数据库、进程间通信 | POSIX `shm_open`+`mmap`、`sem_open`、`mq_open`；匿名用 `memfd_create` | POSIX.1-2001；`memfd` 3.17 | [41.6 节](ch41.md) | 老代码/数据库仍用；新代码优先 POSIX | SysV 对象不是 fd，不能进 epoll |
| POSIX 共享内存/信号量编译要加 `-lrt` | 旧构建脚本、POSIX IPC 示例 | 不写 `-lrt` | glibc 2.43（本机） | [41.6 节](ch41.md) | 老系统/老 glibc 仍可能要；本机不需要 | 链接 `-pthread` 就够 |
| 把 `msgrcv` 的 0 返回值当成功 | 旧 System V 消息队列代码 | 按返回的字节数判断；0 长度消息不是失败 | System V IPC | [41.6 节](ch41.md) | 旧代码判断错；按返回值 >= 0 处理 | `msgrcv` 成功时返回复制进 `mtext` 的字节数 |
| 把 `SEM_UNDO` 当崩溃后的健壮恢复 | 旧 System V 信号量代码 | 健壮互斥量（`PTHREAD_MUTEX_ROBUST`）或重新设计 | POSIX | [41.6 节](ch41.md) | `SEM_UNDO` 只做退出补偿；不能恢复任意崩溃状态 | 健壮互斥量只覆盖互斥量，不覆盖信号量 |
| xinetd 监听端口，每个连接 `fork`/`exec` 子服务 | 旧服务器部署、嵌入式/老机房 | systemd socket 激活（`.socket` + `LISTEN_FDS`） | systemd（版本随发行版） | [41.7 节](ch41.md) | 嵌入式/老系统仍有 xinetd；新部署用 socket 激活 | 不是 POSIX，换环境要自己实现 fd 3 约定 |
| `SIGURG` + `F_SETOWN` 处理带外数据 | 旧 TCP OOB 代码 | `F_SETOWN_EX` + `F_OWNER_TID`；`EPOLLPRI`/`recv(MSG_OOB)` | Linux 2.6.32 | 附录 B；另见 [41.1 节](ch41.md)、[42.5 节](ch42.md) | OOB 很少用；多线程必须定向线程 | 单线程仍可用 `F_SETOWN`；新程序少用 OOB |

### 6. I/O 与事件库（18 行）

| 旧写法 | 典型出处/场景 | 取代它的 | 起始版本 | 本书案例 | 今天还用得着吗 | 备注 |
|---|---|---|---|---|---|---|
| `select` + `FD_SETSIZE` 当无上限 | 旧服务器、I/O 复用教材 | `poll`/`epoll`；每进程上限看 `RLIMIT_NOFILE` | Linux 2.6（epoll）；POSIX（poll） | [42.1 节](ch42.md) | fd 少、要跨平台时 `select`/`poll` 仍够用 | glibc 2.43 的 `FD_SETSIZE=1024`，`FD_SET` 越界是 UB；旧压测还认为单线程 epoll 施压最高，单进程只用一个核 |
| poll/epoll 最多 65535 个 fd | 旧服务器教材、容量规划 | `nfds_t` 是 `unsigned long`；上限是 `RLIMIT_NOFILE` | 无；glibc 2.43 | [42.1 节](ch42.md) | 旧数字不要写死；高并发看 rlimit 和内存 | 系统级上限是 `fs.file-max` |
| 把 `RLIMIT_NOFILE` 说成用户级总限制 | 旧服务器教材、压测章节 | 每进程 `RLIMIT_NOFILE`；系统级 `fs.file-max` | 无 | [42.1 节](ch42.md) | 高并发服务器两个都要看 | `ulimit -n` 改的是每进程限制 |
| `epoll_create(size)` 传连接数当容量 | 旧服务器、epoll 示例 | `epoll_create1(EPOLL_CLOEXEC)` | `size` 自 2.6.8 起被忽略；`epoll_create1` 2.6.27 | [42.2 节](ch42.md) | `epoll_create` 还能用，但没有理由再用 | `epoll_create(0)` 返回 `EINVAL`；只要求 size > 0 |
| `epoll_wait` 是 O(1) | 旧服务器教材、epoll 性能章节 | 成本随本次就绪事件数增长；`epoll_ctl` 也有维护成本 | 无 | [42.2 节](ch42.md) | 旧结论过于简化；容量规划按就绪数算 | `epoll_pwait2`（5.11）给纳秒超时 |
| ET 更高效，所以监听 fd 只 `accept` 一次 | 旧服务器、ET 示例 | 循环 `accept` 到 `EAGAIN`；`accept4` 带 `SOCK_NONBLOCK`/`SOCK_CLOEXEC` | `accept4` 2.6.28 | [42.3 节](ch42.md) | ET 减少重复通知，但要求非阻塞和循环读/写 | 同时到达的两个连接只产生一个边沿 |
| 共享监听 fd 让内核随便唤醒一个 | 旧多进程/多线程服务器 | `EPOLLEXCLUSIVE` 或 `SO_REUSEPORT` | `EPOLLEXCLUSIVE` 4.5；`SO_REUSEPORT` 3.9 | [42.3 节](ch42.md) | 默认唤醒仍可能叫醒多个；按场景选 | `EPOLLEXCLUSIVE` 只能用于 `ADD`，不保证只唤醒一个 |
| 非阻塞 `connect` 后看 `getsockopt` 返回值判断成败 | 旧非阻塞 connect 代码 | 看输出参数 `err`；0 成功，非 0 是连接错误 | 无 | [42.4 节](ch42.md) | 旧代码会把 `ECONNREFUSED` 当成功 | `SO_ERROR` 读一次清零 |
| `SIGIO` + `F_SETOWN` 做信号驱动 I/O | 旧服务器、信号驱动示例 | `epoll`/`io_uring`；多线程用 `F_SETOWN_EX` 定向 | `F_SETOWN_EX` 2.6.32；`io_uring` 5.1 | [42.5 节](ch42.md) | OOB/信号驱动很少用；`EPOLLPRI` 也能报 OOB | 处理函数只能调 async-signal-safe 函数 |
| POSIX AIO 的 `aio_read` 由内核异步搬数据 | 旧服务器、异步 I/O 教材 | `io_uring`（5.1）；文件 I/O 之外不要指望内核 AIO | `io_uring` 5.1 | [42.5 节](ch42.md) | glibc 用用户态辅助线程模拟；本机验证线程数从 1 变 2 | 内核 AIO 对 socket 不是真异步（在 `io_submit` 里同步做完） |
| `mmap` + `writev` 发文件，把 `writev` 返回值当全写完 | 旧服务器、文件发送代码 | 推进 iov 处理部分写；`sendfile`/`splice`/`copy_file_range` | `splice` 2.6.17；`copy_file_range` 4.5；`sendfile` 2.6.33 起文件到文件 | [42.6 节](ch42.md) | 非阻塞 fd 上部分写仍要处理；零拷贝不是默认选项 | 把 `EAGAIN` 当失败会丢剩余数据 |
| `sendfile`/`splice` 是唯一零拷贝 | 旧服务器、零拷贝教程 | `copy_file_range`、`MSG_ZEROCOPY`、`io_uring` 的 `SEND_ZC` | `copy_file_range` 4.5；`MSG_ZEROCOPY` 4.14；`SEND_ZC` 6.0 | [42.6 节](ch42.md) | 静态文件/反向代理仍用 `sendfile`/`splice`；TLS/压缩通常要用户态 | `MSG_ZEROCOPY` 要读错误队列确认完成 |
| `event_init()` 建全局 `current_base` | 旧 Libevent 示例、服务器事件循环 | `event_base_new()` | Libevent 2.0（`event_init` 移入兼容头） | [42.7 节](ch42.md) | `event_init` 仍能编译链接，但新代码用 `event_base_new`；旧代码按兼容头迁移 | 多个 base 不再互相覆盖 |
| Libevent 默认线程安全 | 旧 Libevent 服务器 | `evthread_use_pthreads()` + `-levent_pthreads`，必须在建 base 前调用 | Libevent 2.0 | [42.7 节](ch42.md) | 线程支持只保护 base 内部；回调仍在一个循环线程里串行 | 跨线程 `event_add` 可以，用户数据仍要自己加锁 |
| Libevent 2.0.19 的源码结构/API | 旧 Libevent 源码分析、服务器代码 | 按 2.1.12 的 `event_new`/`event_add`/`event_del` 和 `event_base_*` 迁移 | Libevent 2.1 | [42.7 节](ch42.md) | 2.1 基本兼容 2.0 的核心 API；字段名和内部结构会变 | 2.2 是否存在需核实 |
| Linux 事件源只有 epoll/select/poll/kqueue | 旧 Libevent 章节、I/O 模型综述 | `signalfd`/`eventfd`（2.6.22）、`timerfd`（2.6.25）、`pidfd`（5.3）、`io_uring`（5.1） | Linux 2.6.22–5.3 | [42.7 节](ch42.md) | Libevent 2.1.12 仍未用 `io_uring`；新项目可直接用 | 这些 fd 都能进 epoll，完成式模型另算 |
| TCP SYN 超时约 63 秒、IW 2–4 MSS、默认 reno、keepalive 很快 | 旧 TCP 教材、超时/重试代码 | 读 `/proc/sys/net/ipv4/*` 或 `getsockopt`；`TCP_USER_TIMEOUT` + 应用层心跳 | 本机 `tcp_syn_retries=6`（约 127 秒）；RFC 6928 约 10 MSS | [42.8 节](ch42.md) | 旧数字不要写死；随内核和发行版变 | keepalive 默认 7200 秒 |
| 用 `netstat`/`lsof`/`strace` 排查连接和 fd | 旧服务器运维、监测章节 | `ss`；`strace` 仍可用但受 `ptrace` 限制，`perf trace`/`bpftrace` 可替代 | 工具现状 | 附录 B；另见 [42.1 节](ch42.md) | 老系统仍有 netstat；容器里 strace 常不可用 | 本机没有 strace/perf |

### 7. 生态与工具（11 行）

| 旧写法 | 典型出处/场景 | 取代它的 | 起始版本 | 本书案例 | 今天还用得着吗 | 备注 |
|---|---|---|---|---|---|---|
| 把 C++0x 当未来、把 C++14/17 当最新标准，按“三年一版”讲 | C++11/14 时代的并发书、标准版本综述 | C++20、C++23 已发布；C++26 状态按“发布/标准/实现”分开写 | C++20/C++23；C++26 需核实 | 附录 B；另见 [37.1 节](ch37.md) | 旧书的标准节奏要更新；项目仍可按自己的标准版本维护 | C++26 的 `execution`、风险指针、RCU 本机未实现 |
| C++98 风格：裸 `new[]`、`char*`、`NULL`、`pthread_create` | 旧服务器代码、C++98 教材 | `std::vector`/`std::string`/`std::unique_ptr`；`string_view`/`span`/`jthread` | C++11/17/20 | 附录 B；另见 [42.6 节](ch42.md) | 维护老服务器仍会见到；改造时先换所有权和容器 | 不要为了现代化重写整个 I/O 路径 |
| 事务内存（STM）当 C++ 的下一项同步设施 | C++11 事务内存综述、并发书 | 标准仍无 STM；用互斥量/原子/条件变量 | TM TS 2015；到 C++23 未并入 | 附录 B；另见 [37.4 节](ch37.md) | 没有标准 STM；实验性实现不保证可移植 | C++26 是否纳入需核实 |
| 专用并发测试库/仿真/模型检查是可选加餐 | 旧并发书测试章节 | TSan 常规化 + 压力测试 + 模型检查器/确定性调度器 | 工具现状 | 附录 B；另见 [39.5 节](ch39.md) | 标准库没有并发测试设施；TSan 只覆盖实际路径 | Helgrind/DRD 仍在用；本机没有 |
| 查找表用默认值或 `pair<Value,bool>` 表示“不存在” | C++11 并发映射表示例 | `std::optional<Value>` | C++17 | 附录 B；另见 [37.4 节](ch37.md) | 旧接口要兼容可保留；新代码用 `optional` 表达缺失 | `optional` 不改变锁全部桶的成本 |
| lambda 用 `[=]` 隐式捕获 `this`；`constexpr` 成员函数不能是虚函数 | C++11/14 代码、lambda 教程 | 显式捕获 `this` 或 `[=, this]`；C++20 允许 `constexpr` 虚函数 | C++20 | 附录 B；另见 [37.1 节](ch37.md)、[37.2 节](ch37.md) | 旧代码仍编译但 `[=]` 捕获 `this` 已弃用；新代码写清楚 | C++14 起 `constexpr` 函数体已放宽 |
| 靠移动构造避免返回时的深拷贝 | C++11 值语义示例、工厂函数 | C++17 保证复制消除；具名对象转移仍要 `std::move` | C++17 | 附录 B；另见 [37.1 节](ch37.md) | 旧代码不必改；返回纯右值时不要再写 `std::move` | 纯右值返回不再要求可移动构造 |
| Java 并发只有 `Thread`/`synchronized`/`wait`/`notify`/`atomic` | Java 并发对照、跨语言综述 | `java.util.concurrent` 线程池、`CompletableFuture`；JDK 21 虚拟线程 | JDK 21 | 附录 B；另见 [37.8 节](ch37.md)、[39.6 节](ch39.md) | 跨语言对照仍可用；不要把 Java 监视器等同于 C++ 条件变量 | 虚拟线程适合 I/O 密集，不是 CPU 并行 |
| 延伸阅读停在 MPI-2、OpenMP 3.0，把 GPU 通用计算当成最有前途的方向 | 旧并行教材的延伸阅读、标准版本综述 | MPI 4.0/4.1；OpenMP 5.2 及后续；CUDA/HIP/SYCL/OpenMP target 并存 | MPI-4；OpenMP 5.x | 附录 B；另见 [40.6 节](ch40.md) | 旧书目仍可读；版本和新硬件要另查 | C++ 标准仍无 GPU 接口 |
| Pthreads/信号量是不断发展的 POSIX 标准，C11 threads 会取代它 | 旧 POSIX 综述、跨平台 C 代码 | POSIX.1-2024 已发布；C11 `<threads.h>` 采用不广；Linux 仍 pthread | POSIX.1-2024；glibc 2.34 | [37.1 节](ch37.md) | Linux 新 C 代码仍优先 pthread；可移植性要求高时可用 C11 threads | `-pthread` 仍要写 |
| gdb 只会 `attach` 父进程、看当前线程 | 旧调试章节、多进程服务器 | `set follow-fork-mode`、`set detach-on-fork off`、`info threads`、`thread apply all bt` | gdb 17.1 | [41.4 节](ch41.md) | 多进程/多线程调试仍要这些开关；本机 gdb 17.1 支持 | 默认只跟父进程，子进程栈看不到 |

## 最常被问的十条

1. **`-pthread` 与 `-lpthread`**：新代码一律写 `-pthread`；glibc 2.34 起 libpthread 已并入 libc，但 `-pthread` 仍负责 `_REENTRANT` 和链接。旧 Makefile 里的 `-lpthread` 在老 glibc 上还能链接，不要作为新写法。见 [37.1 节](ch37.md)。
2. **`jthread`**：C++20 起析构先 `request_stop()` 再 `join()`；它不保证线程响应停止，阻塞在 `read` 上的线程仍要让 fd 可读或关闭。见 [37.2 节](ch37.md)。
3. **`stop_token`**：协作停止；作为线程函数的第一个参数自动传入，`condition_variable_any::wait(lock, token, pred)` 能被打断，`condition_variable` 没有这个重载，要用 `stop_callback` 负责 `notify_all()`。见 [37.2 节](ch37.md)。
4. **`atomic::wait`**：C++20；`wait(old)` 在值已不等于 `old` 时立即返回，否则阻塞到被 notify 或虚假唤醒（只改值不会叫醒它），返回后要重查谓词；没有超时版本。见 [38.5 节](ch38.md)。
5. **`atomic_ref`**：C++20；给普通对象加原子访问，要求按 `required_alignment` 对齐，且 `atomic_ref` 存在期间所有访问都走它；不替代栅栏。见 [38.4 节](ch38.md)。
6. **`scoped_lock`**：C++17；一次锁多个，参数个数编译期固定；运行期桶数组仍要按固定顺序 `std::lock`/`try_lock`。见 [37.4 节](ch37.md)。
7. **`move_only_function`**：C++23；能装 `packaged_task` 和捕获 `unique_ptr` 的 lambda；`std::function` 仍要求目标可拷贝。见 [37.7 节](ch37.md)。
8. **`reduce`/`scan`**：C++17；`accumulate`/`partial_sum` 没有执行策略重载；`reduce` 要求结合律和交换律，浮点末位可能不同。见 [40.4 节](ch40.md)。
9. **`signalfd`/`pidfd`/`timerfd`**：Linux 2.6.22/5.3/2.6.25；都能进 `epoll`；`signalfd` 要先屏蔽信号，pidfd 仍要用 `waitid(P_PIDFD)` 回收。见 [41.2 节](ch41.md)、[41.3 节](ch41.md)。
10. **`epoll_create1`**：Linux 2.6.27；`EPOLL_CLOEXEC` 原子设置；`epoll_create(size)` 的 `size` 自 2.6.8 起被忽略，`epoll_wait` 也不是 O(1)。见 [42.2 节](ch42.md)。

## 延伸阅读

- C++ 特性支持：https://en.cppreference.com/w/cpp/compiler_support ，查某个标准特性在 GCC/Clang 上的实现状态。
- Linux man-pages：https://man7.org/linux/man-pages/ ，查系统调用和 glibc 函数的起始版本、错误码和 HISTORY。
- OpenMP 规范：https://www.openmp.org/specifications/ ，`masked`、`task`、`simd` 和调度子句的准确语义。
- MPI 规范：https://www.mpi-forum.org/docs/ ，非阻塞集合、持久集合和大计数接口。
- 各案例的版本依据和延伸阅读见对应章节，本表不重复。