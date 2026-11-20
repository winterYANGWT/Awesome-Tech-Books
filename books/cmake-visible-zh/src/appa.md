# 附录 A 按阶段分的命令速查

## A.1 配置阶段立即执行的命令

配置阶段执行到哪一行，副作用就在哪一行发生：变量、缓存、目标属性当场改变，文件也当场写出。表里的“文件”只算配置时真的落盘的；生成阶段才写或才执行的命令放在 A.2。

| 命令 | 立即产生的副作用（变量/缓存/目标属性/文件） | 章 |
|---|---|---|
| `set(<var> <value>...)` | 在当前作用域写普通变量；`PARENT_SCOPE` 写到上一层；不碰缓存 | [第 3 章](ch03.md)、[第 4 章](ch04.md) |
| `set(<var> <value>... CACHE <type> "<help>" [FORCE])` | 建或改缓存项；没有旧项才写默认值，`FORCE` 才覆盖；无类型项补类型和说明 | [第 5 章](ch05.md) |
| `unset(<var> [CACHE])` | 删当前作用域的普通变量；带 `CACHE` 删缓存项。删掉普通变量后，同名缓存值会露出来 | [第 5 章](ch05.md) |
| `option(<var> "<help>" [<default>])` | 缓存里没有同名项、也没有同名普通变量时写 `BOOL`；省略默认值是 `OFF`；已有普通变量时不动（CMP0077 NEW） | [第 5 章](ch05.md) |
| `list(APPEND/REMOVE_ITEM/REMOVE_DUPLICATES/FILTER/TRANSFORM/SORT/JOIN/FIND ...)` | 读写当前作用域里的列表变量；`FILTER` 从 3.6 起，`TRANSFORM` 从 3.12 起 | [第 3 章](ch03.md)、[第 6 章](ch06.md) |
| `string(REGEX MATCH/REPLACE ... <out>)` | 把结果写进变量；`REGEX MATCH` 同时更新 `CMAKE_MATCH_<n>`；不碰磁盘 | [第 6 章](ch06.md) |
| `math(EXPR <out> "<expr>")` | 把整数结果写进变量 | [第 4 章](ch04.md) |
| `cmake_path(GET/APPEND/RELATIVE_PATH/IS_ABSOLUTE ... <out>)` | 把路径运算结果写进变量；只做语法处理，不查文件是否存在；3.20 起 | [第 6 章](ch06.md) |
| `file(READ <file> <out>)` | 把整个文件读进变量；不会自动登记成配置依赖 | [第 6 章](ch06.md)、[第 12 章](ch12.md) |
| `file(STRINGS <file> <out> [REGEX ...])` | 按行读入列表；`REGEX` 过滤整行，行尾换行被去掉 | [第 6 章](ch06.md) |
| `file(WRITE/APPEND <file> <content>...)` | 立即写或追加；每次配置都执行；相对路径按当前源码目录解析 | [第 12 章](ch12.md) |
| `file(CONFIGURE OUTPUT <file> CONTENT <content> ...)` | 按 `configure_file` 的规则替换后写文件；内容不变不写；相对路径按当前构建目录；3.18 起 | [第 12 章](ch12.md) |
| `file(GLOB <out> [CONFIGURE_DEPENDS] <patterns>...)` | 配置时把匹配到的文件名写成列表；`CONFIGURE_DEPENDS`（3.12）加构建时检查，但并非所有生成器都可靠 | [第 25 章](ch25.md) |
| `configure_file(<in> <out> [@ONLY])` | 在构建目录写出替换后的文件；内容不变不写；输入自动成为重新配置的依赖 | [第 12 章](ch12.md) |
| `execute_process(COMMAND ... OUTPUT_VARIABLE ... RESULT_VARIABLE ...)` | 配置时运行进程，把输出和退出码写进变量；结果保持到下次配置 | [第 12 章](ch12.md) |
| `message([STATUS/VERBOSE/DEBUG/TRACE/CHECK_START/CHECK_PASS/CHECK_FAIL] ...)` | 立即写终端或日志；`FATAL_ERROR` 当场停止配置；VERBOSE、DEBUG、TRACE 默认不显示（3.15 起有这些级别） | [第 1 章](ch01.md)、[第 24 章](ch24.md) |
| `include(<file>)` | 在当前作用域执行被包含的文件；不新建目录或变量作用域；里面的函数、宏定义之后全局可用 | [第 4 章](ch04.md) |
| `include(CTest)` | 配置时加 `BUILD_TESTING` 等缓存项和一批仪表板目标；`BUILD_TESTING` 为 `ON` 时调用 `enable_testing()` | [第 19 章](ch19.md) |
| `add_subdirectory(<dir> [EXCLUDE_FROM_ALL] [SYSTEM])` | 立即读子目录；进入时复制父目录的变量；目标和缓存整个工程共用；`SYSTEM` 从 3.25 起 | [第 4 章](ch04.md)、[第 15 章](ch15.md) |
| `find_package(<pkg> [version] [CONFIG/MODULE] [REQUIRED] [COMPONENTS ...] [GLOBAL])` | 配置时搜索并读包文件；设 `<Pkg>_FOUND`、`<Pkg>_DIR` 等变量或缓存；创建导入目标；`GLOBAL` 从 3.24 起 | [第 14 章](ch14.md) |
| `project(<name> [VERSION ...] [LANGUAGES ...])` | 第一次调用检测编译器、写 `CMakeFiles/<版本>/`，设 `PROJECT_*` 和 `<name>_*`；不写 `LANGUAGES` 时启用 C 和 CXX；3.21 起有 `PROJECT_IS_TOP_LEVEL` | [第 2 章](ch02.md) |
| `cmake_minimum_required(VERSION <min>[...<max>])` | 设最低版本和策略版本；必须放在 `project()` 之前；范围写法从 3.12 起；4.0 起策略版本低于 3.5 直接报错 | [第 2 章](ch02.md)、[第 23 章](ch23.md) |
| `cmake_policy(SET/GET/VERSION ...)` | 改当前策略表；`GET` 把 `NEW`、`OLD` 或空串写进变量；未设置不是 `OLD` | [第 23 章](ch23.md) |
| `enable_language(<lang>)` | 当场检测并启用语言，写编译器检测文件；不创建 `PROJECT_*`；必须在第一次 `project()` 之后、文件作用域里 | [第 2 章](ch02.md) |
| `add_library(<name> [STATIC/SHARED/OBJECT/INTERFACE] ...)` | 建目标并挂源文件、属性；不写类型时看 `BUILD_SHARED_LIBS`，未定义就是 `STATIC`；`ALIAS`、`IMPORTED` 只登记名字 | [第 7 章](ch07.md)、[第 11 章](ch11.md) |
| `add_executable(<name> ...)` | 建可执行目标并挂源文件、属性；`IMPORTED`、`ALIAS` 形式只登记 | [第 7 章](ch07.md) |
| `target_sources(<t> <keyword> ... [FILE_SET HEADERS ...])` | 写源文件列表和 `INTERFACE_SOURCES`；`FILE_SET HEADERS` 从 3.23 起 | [第 16 章](ch16.md)、[第 26 章](ch26.md) |
| `target_include_directories(<t> <keyword> <dirs>...)` | `PRIVATE` 写 `INCLUDE_DIRECTORIES`，`INTERFACE` 写 `INTERFACE_INCLUDE_DIRECTORIES`，`PUBLIC` 两个都写 | [第 8 章](ch08.md) |
| `target_compile_definitions(<t> <keyword> <defs>...)` | 同样按关键字写 `COMPILE_DEFINITIONS` 和 `INTERFACE_COMPILE_DEFINITIONS` | [第 8 章](ch08.md) |
| `target_compile_options(<t> <keyword> <opts>...)` | 按关键字写 `COMPILE_OPTIONS` 和 `INTERFACE_COMPILE_OPTIONS`；真正进编译命令要等构建 | [第 8 章](ch08.md)、[第 10 章](ch10.md) |
| `target_compile_features(<t> <keyword> <features>...)` | 按关键字写 `COMPILE_FEATURES` 和 `INTERFACE_COMPILE_FEATURES`；默认标准已满足时不加 `-std` | [第 10 章](ch10.md) |
| `target_link_libraries(<t> <keyword> <libs>...)` | 写 `LINK_LIBRARIES` 和 `INTERFACE_LINK_LIBRARIES`；静态库的 `PRIVATE` 依赖以 `$<LINK_ONLY:...>` 留在接口里 | [第 8 章](ch08.md) |
| `target_link_options(<t> <keyword> <opts>...)` | 按关键字写 `LINK_OPTIONS` 和 `INTERFACE_LINK_OPTIONS`；3.13 起 | [第 21 章](ch21.md) |
| `set_target_properties(<t> PROPERTIES <prop> <value>...)` | 立即改目标属性；`OUTPUT_NAME` 改产物名，`VERSION`、`SOVERSION` 影响链接命令里的 soname，构建时建符号链接 | [第 7 章](ch07.md)、[第 11 章](ch11.md) |
| `set_property(TARGET <t> [APPEND] PROPERTY <prop> <values>...)` | 立即覆盖或追加目标属性；`APPEND` 只对列表属性合适 | [第 7 章](ch07.md) |
| `set_property(DIRECTORY [APPEND] PROPERTY CMAKE_CONFIGURE_DEPENDS <files>...)` | 把额外文件登记成配置依赖；变量、缓存、目标栏都没有变化 | [第 12 章](ch12.md) |
| `include_directories(<dirs>...)` | 追加当前目录的 `INCLUDE_DIRECTORIES`；当前目录已有的目标也追加；之后进入的子目录继承 | [第 8 章](ch08.md)、[第 25 章](ch25.md) |
| `add_compile_options(<opts>...)` | 追加当前目录的 `COMPILE_OPTIONS`；只影响之后创建的目标，子目录在进入时继承 | [第 8 章](ch08.md) |
| `add_definitions(<defs>...)` | 追加当前目录的 `COMPILE_DEFINITIONS`；生成时对整个目录的目标生效，目标自己的属性仍为空 | [第 8 章](ch08.md) |
| `link_libraries(<libs>...)` | 追加当前目录的 `LINK_LIBRARIES`；只影响之后创建的目标，子目录在进入时继承 | [第 8 章](ch08.md) |
| `add_link_options(<opts>...)` | 追加当前目录的 `LINK_OPTIONS`；只影响之后创建的目标；3.13 起 | [第 26 章](ch26.md) |
| `generate_export_header(<t> ...)` | 配置时在构建目录写 `<t>_export.h`，定义导出和隐藏宏 | [第 11 章](ch11.md)、[第 26 章](ch26.md) |
| `configure_package_config_file(<in> <out> ...)` | 配置时写出 `XxxConfig.cmake`，展开 `@PACKAGE_INIT@` 等占位符 | [第 17 章](ch17.md) |
| `write_basic_package_version_file(<out> VERSION ... COMPATIBILITY ...)` | 配置时写出 `XxxConfigVersion.cmake`，把兼容策略写成可执行的比较 | [第 17 章](ch17.md)、[第 18 章](ch18.md) |
| `FetchContent_Declare(<name> ...)` | 只登记来源、哈希和 `FIND_PACKAGE_ARGS`（3.24 起）；不下载，不建目标 | [第 15 章](ch15.md) |
| `FetchContent_MakeAvailable(<name>)` | 配置时先按声明尝试 `find_package`，否则下载、解压并 `add_subdirectory`；目标和缓存进本工程 | [第 15 章](ch15.md) |

## A.2 只登记、以后才执行的命令

这些命令在配置页通常什么也不改，只是把动作写进构建、测试或安装脚本。判断标准是：配置阶段有没有看得见的变量、缓存、目标属性或文件变化。

| 命令 | 登记到哪里（build.ninja / CTestTestfile.cmake / cmake_install.cmake） | 真正执行的阶段 | 章 |
|---|---|---|---|
| `file(GENERATE OUTPUT <out> CONTENT <content> [TARGET <t>])` | 生成阶段直接写文件，不进 `build.ninja`；`TARGET` 从 3.19 起 | 生成；多配置生成器对每个配置各写一份；内容不变不重写 | [第 9 章](ch09.md)、[第 12 章](ch12.md) |
| `add_custom_command(OUTPUT <files> COMMAND ... DEPENDS ... [BYPRODUCTS ...])` | `build.ninja` 的文件生成规则 | 构建；输出缺失或 `DEPENDS` 比输出新时重跑 | [第 13 章](ch13.md) |
| `add_custom_command(TARGET <t> PRE_BUILD/PRE_LINK/POST_BUILD COMMAND ...)` | 目标自己的构建规则，写在 `build.ninja` 里 | 构建；目标被重建时执行，`PRE_BUILD` 在非 Visual Studio 生成器下等同 `PRE_LINK` | [第 13 章](ch13.md) |
| `add_custom_target(<name> [ALL] COMMAND ...)` | `build.ninja` 里的 UTILITY 目标 | 每次构建这个目标都执行；没有输出文件，`ALL` 只决定进不进默认构建 | [第 13 章](ch13.md) |
| `add_dependencies(<t> <deps>...)` | `build.ninja` 里的目标级依赖 | 构建；只保证顺序，不建立文件级依赖 | [第 13 章](ch13.md) |
| `enable_testing()` | 生成阶段把测试表写进 `CTestTestfile.cmake` | 测试；ctest 从构建目录顶层读清单 | [第 19 章](ch19.md) |
| `add_test(NAME <name> COMMAND <cmd> ...)` | `CTestTestfile.cmake` 里的一条测试 | 测试；ctest 运行时起进程，不构建；目标名在生成时换成绝对路径 | [第 19 章](ch19.md) |
| `set_tests_properties(<tests> PROPERTIES <prop> <value>...)` | 同一份 `CTestTestfile.cmake` | 测试；ctest 调度和判分时读这些属性 | [第 19 章](ch19.md) |
| `gtest_discover_tests(<t> ...)` | 配置时写 `t[N]_include.cmake`；构建的 POST_BUILD 阶段再写 `t[N]_tests.cmake` | 构建时列用例，ctest 运行时执行；要先打开测试 | [第 19 章](ch19.md) |
| `install(TARGETS <t>... [EXPORT <set>] [FILE_SET HEADERS ...] [DESTINATION ...] [COMPONENT ...])` | 生成阶段写进 `cmake_install.cmake` | 安装；`cmake --install` 时复制，配置页没有文件变化 | [第 16 章](ch16.md) |
| `install(FILES/DIRECTORY/PROGRAMS ...)` | 生成阶段写进 `cmake_install.cmake` | 安装；按规则复制文件或目录树 | [第 16 章](ch16.md) |
| `install(CODE ...)`、`install(SCRIPT <file>)` | 生成阶段写进 `cmake_install.cmake` | 安装；按声明顺序执行，变量到安装时才求值 | [第 16 章](ch16.md) |
| `install(EXPORT <set> [NAMESPACE ...] DESTINATION ...)` | 生成阶段写 `build/CMakeFiles/Export/...`；安装时复制到前缀 | 生成阶段写文件，`cmake --install` 复制；下游 `find_package` 时读 | [第 17 章](ch17.md) |
| `export(EXPORT <set>/TARGETS <t>... [NAMESPACE ...] FILE <file>)` | 生成阶段直接写构建树的 Targets 文件，不经过 `build.ninja`，也不安装 | 下游 `include` 这个文件时使用 | [第 17 章](ch17.md) |
| `$<...>` 生成器表达式 | 先原样存进目标属性；生成阶段对每个目标、每种配置求值，写进 `build.ninja` 或 `file(GENERATE)` 的文件 | 生成；配置阶段的 `if()`、`message()` 只看到原文 | [第 9 章](ch09.md) |

## A.3 命令行和环境变量

命令行选项在 CMake 读第一行之前生效；环境变量只在对应参数没给时补默认值。版本号按 CMake 4.2.3 的文档核对。

| 写法 | 作用 | 注意 | 章 |
|---|---|---|---|
| `cmake -S <src> -B <build>` | 配置并生成；`-B` 目录不存在时会创建 | 构建目录不要和源码目录相同；`-S`、`-B` 从 3.13 起 | [第 1 章](ch01.md) |
| `cmake -S <src> -B <build> -G <generator>` | 指定生成器，写进缓存 | 同一构建目录换生成器会报错；要换就新建目录或 `--fresh` | [第 1 章](ch01.md) |
| `cmake -B <build> -D<var>[:<type>]=<value>` | 配置开始前建或改缓存项；不带类型时是 `UNINITIALIZED` | 命令行优先于缓存旧值和 preset；工程用 `set(CACHE PATH/FILEPATH)` 补类型时相对路径会转成绝对路径 | [第 5 章](ch05.md) |
| `cmake -B <build> -U <globbing_expr>` | 配置前删除匹配的缓存项 | 支持 `*`、`?`；删多了会让 `CMakeCache.txt` 不可用 | [第 5 章](ch05.md) |
| `cmake --fresh -B <build>` | 删掉 `CMakeCache.txt` 和 `CMakeFiles/` 后从头配置 | 3.24 起；用于换编译器、换生成器或回到干净状态 | [第 1 章](ch01.md)、[第 5 章](ch05.md) |
| `cmake --preset <name>` | 在其它命令行选项之前读预设，设置生成器、构建目录、缓存和环境 | 预设里的值可被命令行覆盖；`hidden` 预设不能直接选用 | [第 20 章](ch20.md) |
| `cmake --list-presets[=<kind>]` | 列出可用的配置、构建、测试、打包或工作流预设 | 在含 `CMakePresets.json` 的目录里运行 | [第 20 章](ch20.md) |
| `cmake --toolchain <file>` 或 `cmake -B <build> -DCMAKE_TOOLCHAIN_FILE=<file>` | 指定工具链文件，第一次 `project()` 时读，路径写进缓存 | 3.21 起；相对路径先按构建目录、再按源码目录解析 | [第 22 章](ch22.md) |
| `cmake --log-level=<level>` | 设置 `message()` 的显示级别 | 3.16 起；默认 `STATUS`；命令行优先于 `CMAKE_MESSAGE_LOG_LEVEL` | [第 24 章](ch24.md) |
| `cmake --trace-expand [--trace-source=<file>] [--trace-format=json-v1]` | 打印每条命令，变量已展开；可只看一个文件或输出 JSON | `--trace-expand` 从 3.4 起，`--trace-source` 从 3.7 起，`--trace-format` 从 3.17 起 | [第 24 章](ch24.md) |
| `cmake --debug-find`、`--debug-find-pkg=<pkg>`、`--debug-find-var=<var>` | 打印 `find_*` 的查找过程和试过的目录 | `--debug-find` 从 3.17 起，两个过滤参数从 3.23 起 | [第 24 章](ch24.md) |
| `cmake --graphviz=<file>` | 生成目标依赖的 Graphviz 文件，同时写每目标的子文件 | 3.10 起区分 PUBLIC、PRIVATE、INTERFACE 的线型；画图还要装 Graphviz | [第 24 章](ch24.md) |
| `cmake --debugger --debugger-pipe=<pipe>` | 启动 CMake 语言的 DAP 调试器 | 3.27 起；需要客户端下断点、单步、读变量 | [第 24 章](ch24.md) |
| `cmake --build <build>` | 调用本机构建工具 | Ninja 发现 `build.ninja` 比 CMake 输入旧时，会先执行 `--regenerate-during-build` | [第 1 章](ch01.md) |
| `cmake --build <build> --target <t>` | 只构建指定目标 | 目标名来自生成页；未写 `ALL` 的自定义目标通常靠它构建 | [第 1 章](ch01.md)、[第 13 章](ch13.md) |
| `cmake --build <build> -j <N>` | 最多并行 N 个构建任务 | 3.12 起；不写 N 用本机默认 | [第 1 章](ch01.md) |
| `cmake --build <build> --verbose` | 打印每条真实命令 | 查参数从哪来时先看这个 | [第 1 章](ch01.md) |
| `cmake --build <build> --clean-first` | 先清理再构建 | 清理范围由生成器决定 | [第 1 章](ch01.md) |
| `cmake --build <build> --config <cfg>` | 多配置生成器选择配置 | 单配置生成器用 `CMAKE_BUILD_TYPE`，这个参数不参与选择 | [第 9 章](ch09.md) |
| `cmake --install <build>` | 执行 `cmake_install.cmake`，复制安装规则点名的文件 | 配置阶段只登记，真正碰磁盘的是这一步 | [第 16 章](ch16.md) |
| `cmake --install <build> --prefix <prefix>` | 覆盖 `CMAKE_INSTALL_PREFIX` | 只换前缀；绝对 `DESTINATION` 不受影响 | [第 16 章](ch16.md) |
| `cmake --install <build> --component <comp>` | 只安装指定组件的规则 | 没写组件的规则属于 `Unspecified` | [第 16 章](ch16.md) |
| `cmake --workflow --preset <name>` | 按 workflow 预设依次执行配置、构建、测试或打包 | 3.25 起；第一步必须是 configure；可加 `--fresh` | [第 20 章](ch20.md)、[第 26 章](ch26.md) |
| `ctest --test-dir <build>` | 指定构建目录，读 `CTestTestfile.cmake` | 3.20 起；省掉先 `cd build` | [第 19 章](ch19.md) |
| `ctest -R <regex>` | 只跑名字匹配的测试 | 按测试名匹配，不是按源文件 | [第 19 章](ch19.md) |
| `ctest -L <regex>` | 只跑标签匹配的测试 | 会把被选中测试依赖的 fixture setup、cleanup 一起带上 | [第 19 章](ch19.md) |
| `ctest -j <N>` | 最多并行 N 个测试 | 默认串行；共用文件的测试要自己错开 | [第 19 章](ch19.md) |
| `ctest --output-on-failure` | 失败时打印测试自己的输出 | 通过的测试不打印；`-V` 才打印全部 | [第 19 章](ch19.md) |
| `ctest --rerun-failed` | 只重跑上一次失败的测试 | 会忽略 `-R`、`-L`、`-E` 等筛选参数 | [第 19 章](ch19.md) |
| `ctest -N` | 只列测试，不启动进程 | 用来确认清单；没有构建时读到的是旧清单 | [第 19 章](ch19.md) |
| `CC`、`CXX` | 第一次配置时选择 C、C++ 编译器，结果写进 `CMAKE_<LANG>_COMPILER` | 已定义 `CMAKE_<LANG>_COMPILER` 时忽略；值里可以带编译器选项 | [第 2 章](ch02.md) |
| `CMAKE_GENERATOR` | 没有 `-G` 时指定默认生成器 | 3.15 起；无效值退回内置默认；结果存进 `CMAKE_GENERATOR` | [第 1 章](ch01.md) |
| `CMAKE_BUILD_PARALLEL_LEVEL` | 给 `cmake --build` 提供默认并行数 | 3.12 起；空值表示用本机构建工具的默认值 | [第 1 章](ch01.md) |
| `CMAKE_PREFIX_PATH` | 给 `find_package`、`find_*` 一组安装前缀 | UNIX 上用 `:` 分隔，Windows 上用 `;`；不是配置文件目录 | [第 14 章](ch14.md) |
| `CMAKE_EXPORT_COMPILE_COMMANDS` | 第一次配置时给同名变量提供默认值 | 3.17 起；只在 Makefile、Ninja 生成器下写 `compile_commands.json` | [第 21 章](ch21.md) |
| `CMAKE_TOOLCHAIN_FILE` | 第一次配置时给同名缓存变量提供默认值 | 3.21 起；命令行 `--toolchain` 优先 | [第 22 章](ch22.md) |
