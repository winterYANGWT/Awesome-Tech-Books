# 附录 B 常用变量和目标属性

这一篇把正文里出现过的变量和目标属性按用途集中起来，每一行末尾标出讲它的章节。路径例子沿用查看器里的 `/work/src`、`/work/build`。

## B.1 路径变量

同一组变量在顶层、`add_subdirectory()` 的子目录、`include()` 的文件和函数体内分别指向哪里，先看这张表；`helpers.cmake` 放在 `cmake/` 目录。`CMAKE_CURRENT_LIST_DIR` 在函数里指向调用者的文件，要定位函数定义旁边的模板得用 `CMAKE_CURRENT_FUNCTION_LIST_DIR`（3.17 起）。

| 变量 | 顶层 | 子目录 | `include()` 的文件 | 函数体内 | 章 |
|---|---|---|---|---|---|
| `CMAKE_SOURCE_DIR` | `/work/src` | `/work/src` | `/work/src` | `/work/src` | [第 1 章](ch01.md)、[第 4 章](ch04.md) |
| `CMAKE_BINARY_DIR` | `/work/build` | `/work/build` | `/work/build` | `/work/build` | [第 1 章](ch01.md)、[第 4 章](ch04.md) |
| `CMAKE_CURRENT_SOURCE_DIR` | `/work/src` | `/work/src/sub` | `/work/src`（不变） | 调用者所在目录 | [第 4 章](ch04.md) |
| `CMAKE_CURRENT_BINARY_DIR` | `/work/build` | `/work/build/sub` | `/work/build`（不变） | 调用者的构建目录 | [第 4 章](ch04.md) |
| `CMAKE_CURRENT_LIST_DIR` | `/work/src` | `/work/src/sub` | `/work/src/cmake` | 调用者正在处理的文件所在目录 | [第 4 章](ch04.md)、[第 6 章](ch06.md) |
| `CMAKE_CURRENT_LIST_FILE` | `/work/src/CMakeLists.txt` | `/work/src/sub/CMakeLists.txt` | `/work/src/cmake/helpers.cmake` | 调用者正在处理的文件 | [第 4 章](ch04.md) |
| `PROJECT_SOURCE_DIR` | `/work/src` | `/work/src/sub`（有 `project(Sub)` 时） | `/work/src` | 调用者目录作用域最近一次 `project()` | [第 2 章](ch02.md) |
| `PROJECT_BINARY_DIR` | `/work/build` | `/work/build/sub`（有 `project(Sub)` 时） | `/work/build` | 同上 | [第 2 章](ch02.md) |
| `CMAKE_CURRENT_FUNCTION_LIST_DIR` | — | — | — | `/work/src/cmake`（函数定义处） | [第 6 章](ch06.md) |
| `CMAKE_CURRENT_FUNCTION_LIST_FILE` | — | — | — | `/work/src/cmake/helpers.cmake` | [第 6 章](ch06.md) |

`PROJECT_SOURCE_DIR` 看当前目录作用域最近一次 `project()`，子目录里的 `project()` 不会改父目录的值。

## B.2 编译器和平台

编译器变量在第一次 `project()`（或 `enable_language()`）检测语言时写入，之后换编译器要新构建目录。

| 变量 | 含义 | 本机（GCC 15 / Linux） | 备注 | 章 |
|---|---|---|---|---|
| `CMAKE_CXX_COMPILER` | C++ 编译器完整路径 | `/usr/bin/g++` | 缓存项，类型 `FILEPATH`；设过不能改 | [第 2 章](ch02.md) |
| `CMAKE_CXX_COMPILER_ID` | 编译器厂商标识 | `GNU` | 常见还有 `Clang`、`MSVC`、`AppleClang` | [第 2 章](ch02.md)、[第 10 章](ch10.md) |
| `CMAKE_CXX_COMPILER_VERSION` | 编译器版本 | `15.2.0` | 文档格式是 `major[.minor[.patch[.tweak]]]` | [第 2 章](ch02.md) |
| `CMAKE_CXX_SIMULATE_ID` | 被模拟编译器的 ID（ABI 兼容） | 空 | clang-cl 为 `MSVC`；变量名没有 `COMPILER` | [第 10 章](ch10.md) |
| `CMAKE_CXX_COMPILER_FRONTEND_VARIANT` | 命令行前端变体 | `GNU` | 变量 3.14 起；GNU/MSVC 从 3.26 起也设；clang-cl 为 `MSVC` | [第 10 章](ch10.md) |

`SIMULATE_ID` 的完整名字是 `CMAKE_CXX_SIMULATE_ID`，文档写作 `CMAKE_<LANG>_SIMULATE_ID`，不是 `CMAKE_CXX_COMPILER_SIMULATE_ID`。平台变量描述的是目标系统，不是运行 CMake 的机器；交叉编译时由工具链文件设。

| 变量 | 含义 | 本机 | 备注 | 章 |
|---|---|---|---|---|
| `CMAKE_SYSTEM_NAME` | 目标系统名 | `Linux` | 手动设置即进入交叉编译 | [第 2 章](ch02.md)、[第 22 章](ch22.md) |
| `CMAKE_SYSTEM_VERSION` | 目标系统版本 | 随主机 | 设 `CMAKE_SYSTEM_NAME` 时必须一起设 | [第 22 章](ch22.md) |
| `CMAKE_SYSTEM_PROCESSOR` | 目标处理器 | `x86_64` | 交叉时由工具链文件给出 | [第 2 章](ch02.md)、[第 22 章](ch22.md) |
| `CMAKE_CROSSCOMPILING` | 是否交叉编译 | `FALSE` | 手动设 `SYSTEM_NAME` 后为 `TRUE`，同名也算 | [第 2 章](ch02.md)、[第 22 章](ch22.md) |
| `UNIX` | 目标是否 UNIX 或类 UNIX | `TRUE` | Linux、macOS、Cygwin 为真 | [第 2 章](ch02.md)、[第 22 章](ch22.md) |
| `WIN32` | 目标是否 Windows（含 Win64） | `FALSE` | 与宿主无关 | [第 2 章](ch02.md) |
| `APPLE` | 目标是否 Apple 平台 | `FALSE` | macOS、iOS、tvOS、visionOS、watchOS | [第 2 章](ch02.md)、[第 22 章](ch22.md) |
| `MSVC` | 编译器是否 MSVC 或模拟 cl 前端 | `FALSE` | clang-cl 也为真；参数风格看 `FRONTEND_VARIANT` | [第 10 章](ch10.md)、[第 22 章](ch22.md) |
| `CMAKE_HOST_SYSTEM_NAME` | 运行 CMake 的机器系统 | `Linux` | 和 `CMAKE_SYSTEM_NAME` 对照看 | [第 22 章](ch22.md) |

## B.3 项目和构建类型

`PROJECT_*` 由 `project()` 写入，跟着目录作用域走；`CMAKE_PROJECT_*` 记顶层项目的值，`<名>_SOURCE_DIR`、`<名>_BINARY_DIR`、`<名>_IS_TOP_LEVEL` 是缓存项。

| 变量 | 由谁设置 | 值 / 范围 | 章 |
|---|---|---|---|
| `PROJECT_NAME` | `project()` | 当前目录作用域最近一次 `project()` 的名字；回到父目录恢复 | [第 2 章](ch02.md) |
| `PROJECT_SOURCE_DIR`、`PROJECT_BINARY_DIR` | `project()` | 最近一次 `project()` 的源码 / 构建目录；子目录的 `project()` 不改父目录 | [第 2 章](ch02.md) |
| `PROJECT_VERSION`、`PROJECT_VERSION_MAJOR` 等 | `project(VERSION ...)` | 版本和拆开的组件 | [第 2 章](ch02.md)、[第 6 章](ch06.md) |
| `PROJECT_IS_TOP_LEVEL` | `project()`（3.21 起） | 最近一次 `project()` 是否在顶层 | [第 2 章](ch02.md)、[第 15 章](ch15.md) |
| `<名>_SOURCE_DIR`、`<名>_BINARY_DIR` | `project()` | 按项目名固定；缓存项，类型 `STATIC` | [第 2 章](ch02.md) |
| `<名>_IS_TOP_LEVEL` | `project()`（3.21 起） | 该项目是否在顶层；缓存项，类型 `STATIC` | [第 2 章](ch02.md) |
| `<名>_VERSION` | `project(VERSION ...)` | 按项目名的版本；普通变量，不进缓存 | [第 2 章](ch02.md) |
| `CMAKE_PROJECT_NAME` | 顶层 `project()` | 顶层项目名；缓存项，类型 `STATIC`；子目录改不了 | [第 2 章](ch02.md) |
| `CMAKE_PROJECT_VERSION` | 顶层 `project(VERSION ...)`（3.12 起） | 顶层版本；缓存项，类型 `STATIC` | [第 2 章](ch02.md) |

构建类型和 flags 由生成器与缓存决定，下面几个变量最容易在命令行和工程之间打架。

| 变量 | 作用 | 默认 / 本机 | 章 |
|---|---|---|---|
| `CMAKE_BUILD_TYPE` | 单配置生成器的构建类型 | 不传时通常为空；`CMAKE_BUILD_TYPE` 环境变量或工具链可给默认 | [第 10 章](ch10.md) |
| `CMAKE_CONFIGURATION_TYPES` | 多配置生成器的配置列表 | 由生成器决定；Ninja Multi-Config 本机为 `Debug;Release;RelWithDebInfo` | [第 9 章](ch09.md) |
| `CMAKE_CXX_FLAGS` | 所有配置的 C++ 编译和链接 flags | 由 `CXXFLAGS` 初始化并写进缓存；按目录作用域，当前目录的目标都读得到，之后进入的子目录读不到 | [第 10 章](ch10.md) |
| `CMAKE_CXX_FLAGS_<CONFIG>` | 某一配置追加的 flags | 在 `CMAKE_CXX_FLAGS` 之后；GCC 的 `Debug=-g`、`Release=-O3 -DNDEBUG`、`RelWithDebInfo=-O2 -g -DNDEBUG`、`MinSizeRel=-Os -DNDEBUG` | [第 10 章](ch10.md) |
| `BUILD_SHARED_LIBS` | 没写类型的 `add_library()` 默认库类型 | 默认未定义，按 `STATIC`；常用 `option()` 建缓存项 | [第 11 章](ch11.md) |
| `CMAKE_INSTALL_PREFIX` | 安装前缀 | Linux 默认 `/usr/local`；`cmake --install --prefix` 可覆盖；3.29 起环境变量可给默认 | [第 16 章](ch16.md) |
| `CMAKE_PREFIX_PATH` | `find_package()` 等查找的安装前缀列表 | 默认空；分号分隔的列表，不是配置文件目录 | [第 14 章](ch14.md) |
| `CMAKE_MODULE_PATH` | `include()`、`find_package()` 先找模块的目录 | 默认空；项目自己设 | [第 14 章](ch14.md) |
| `CMAKE_DEFAULT_BUILD_TYPE` | Ninja Multi-Config 的默认配置 | 不设时用 `CMAKE_CONFIGURATION_TYPES` 第一项；值必须是其中一项；3.17 起 | [第 9 章](ch09.md)、[第 10 章](ch10.md) |

`CMAKE_BUILD_TYPE` 和 `CMAKE_CONFIGURATION_TYPES` 各管一种生成器，不要同时依赖。

## B.4 目标属性

目标属性分两半：`INCLUDE_DIRECTORIES` 这类给自己编译用，`INTERFACE_INCLUDE_DIRECTORIES` 这类在别人链接时传给对方。`PUBLIC` 同时写两半，`PRIVATE` 只写左边，`INTERFACE` 只写右边。

| 用途 | 自己用的属性 | 传给下游的 `INTERFACE_*` | 写它的命令 | 章 |
|---|---|---|---|---|
| 头文件搜索目录 | `INCLUDE_DIRECTORIES` | `INTERFACE_INCLUDE_DIRECTORIES` | `target_include_directories()` | [第 8 章](ch08.md) |
| 预处理宏 | `COMPILE_DEFINITIONS` | `INTERFACE_COMPILE_DEFINITIONS` | `target_compile_definitions()` | [第 8 章](ch08.md) |
| 编译选项 | `COMPILE_OPTIONS` | `INTERFACE_COMPILE_OPTIONS` | `target_compile_options()` | [第 8 章](ch08.md)、[第 10 章](ch10.md) |
| 语言特性 | `COMPILE_FEATURES` | `INTERFACE_COMPILE_FEATURES` | `target_compile_features()` | [第 8 章](ch08.md)、[第 10 章](ch10.md) |
| 链接库 | `LINK_LIBRARIES` | `INTERFACE_LINK_LIBRARIES` | `target_link_libraries()` | [第 8 章](ch08.md) |
| 链接选项 | `LINK_OPTIONS` | `INTERFACE_LINK_OPTIONS` | `target_link_options()` | [第 21 章](ch21.md) |
| 链接目录 | `LINK_DIRECTORIES` | `INTERFACE_LINK_DIRECTORIES` | `target_link_directories()` | [第 25 章](ch25.md) |

静态库的 `target_link_libraries(x PRIVATE y)` 会在 `INTERFACE_LINK_LIBRARIES` 里留下 `$<LINK_ONLY:y>`，只把 y 带进最终链接；`LINK_OPTIONS`、`LINK_DIRECTORIES` 这两个目标属性（以及它们的 `INTERFACE_*`）从 3.13 起。下面这些属性只在创建目标的那一刻从同名 `CMAKE_<PROP>` 变量取初值，之后改变量不会回头改已有目标。

| 目标属性 | 初始化变量 | 默认 / 备注 | 章 |
|---|---|---|---|
| `CXX_STANDARD` | `CMAKE_CXX_STANDARD` | 3.1 起；不设时用编译器默认 | [第 7 章](ch07.md)、[第 10 章](ch10.md) |
| `CXX_STANDARD_REQUIRED` | `CMAKE_CXX_STANDARD_REQUIRED` | 默认不要求（`OFF` 或未设），标准可以退回 | [第 10 章](ch10.md) |
| `CXX_EXTENSIONS` | `CMAKE_CXX_EXTENSIONS` | 变量不设时：CMP0128 NEW 取 `CMAKE_CXX_EXTENSIONS_DEFAULT`，旧行为为 `ON` | [第 10 章](ch10.md) |
| `POSITION_INDEPENDENT_CODE` | `CMAKE_POSITION_INDEPENDENT_CODE` | 只初始化非 `SHARED`/`MODULE` 目标；`SHARED`/`MODULE` 默认 `ON` | [第 7 章](ch07.md)、[第 11 章](ch11.md) |
| `CXX_VISIBILITY_PRESET` | `CMAKE_CXX_VISIBILITY_PRESET` | 不设时不加 `-fvisibility` | [第 7 章](ch07.md)、[第 11 章](ch11.md) |
| `VISIBILITY_INLINES_HIDDEN` | `CMAKE_VISIBILITY_INLINES_HIDDEN` | 不设时不加 `-fvisibility-inlines-hidden` | [第 11 章](ch11.md) |
| `RUNTIME_OUTPUT_DIRECTORY` | `CMAKE_RUNTIME_OUTPUT_DIRECTORY` | 多配置生成器会自动加配置子目录 | [第 6 章](ch06.md)、[第 7 章](ch07.md) |
| `LIBRARY_OUTPUT_DIRECTORY` | `CMAKE_LIBRARY_OUTPUT_DIRECTORY` | 多配置生成器会自动加配置子目录 | [第 7 章](ch07.md) |
| `ARCHIVE_OUTPUT_DIRECTORY` | `CMAKE_ARCHIVE_OUTPUT_DIRECTORY` | 多配置生成器会自动加配置子目录 | [第 7 章](ch07.md) |
| `INSTALL_RPATH` | `CMAKE_INSTALL_RPATH` | 默认空；安装时替换构建树 rpath | [第 11 章](ch11.md)、[第 16 章](ch16.md) |
| `BUILD_RPATH` | `CMAKE_BUILD_RPATH` | 属性是额外目录，自动值由 CMake 生成 | [第 11 章](ch11.md) |
| `CXX_COMPILER_LAUNCHER` | `CMAKE_CXX_COMPILER_LAUNCHER` | 3.4 起；创建目标时复制 | [第 21 章](ch21.md) |
| `CXX_CLANG_TIDY` | `CMAKE_CXX_CLANG_TIDY` | 3.6 起；设成空串可逐目标关闭 | [第 21 章](ch21.md) |
| `CXX_SCAN_FOR_MODULES` | `CMAKE_CXX_SCAN_FOR_MODULES` | 3.28 起；不设时 C++20 及以上目标按 CMP0155 扫描 | [第 10 章](ch10.md) |
| `CROSSCOMPILING_EMULATOR` | `CMAKE_CROSSCOMPILING_EMULATOR` | 3.3 起；只在交叉编译时用 | [第 22 章](ch22.md) |

`OUTPUT_NAME` 没有对应的 `CMAKE_OUTPUT_NAME` 变量，默认用目标名；这类属性只能逐个目标设。
