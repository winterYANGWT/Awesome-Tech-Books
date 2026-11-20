# Summary

[前言：这本书怎么读](index.md)

# 第一部分 CMake 到底在做什么

- [第 1 章 三个阶段：配置、生成、构建](ch01.md)
- [第 2 章 project() 做了什么](ch02.md)
- [第 3 章 CMake 语言：一切都是字符串](ch03.md)

# 第二部分 变量、作用域和缓存

- [第 4 章 作用域：目录、函数、宏、include()、block()](ch04.md)
- [第 5 章 缓存：为什么“改了不生效”](ch05.md)
- [第 6 章 函数、参数解析和字符串列表](ch06.md)

# 第三部分 目标是中心

- [第 7 章 目标和属性](ch07.md)
- [第 8 章 用法要求：PUBLIC、PRIVATE、INTERFACE](ch08.md)
- [第 9 章 生成器表达式](ch09.md)
- [第 10 章 编译选项、语言标准和构建类型](ch10.md)
- [第 11 章 库：静态、动态和符号可见性](ch11.md)

# 第四部分 在配置时和构建时做事

- [第 12 章 配置时做事：configure_file、file()、execute_process](ch12.md)
- [第 13 章 构建时做事：add_custom_command 和 add_custom_target](ch13.md)

# 第五部分 依赖

- [第 14 章 find_package：模块模式和配置模式](ch14.md)
- [第 15 章 把依赖带进构建：子目录、FetchContent、先找再下载](ch15.md)

# 第六部分 安装和导出

- [第 16 章 安装：install() 和 GNUInstallDirs](ch16.md)
- [第 17 章 导出目标，让别人 find_package 你](ch17.md)
- [第 18 章 证明它能用：安装后消费测试](ch18.md)

# 第七部分 测试和工程化

- [第 19 章 CTest](ch19.md)
- [第 20 章 预设：CMakePresets.json](ch20.md)
- [第 21 章 质量工具接进构建](ch21.md)
- [第 22 章 工具链文件和交叉编译](ch22.md)

# 第八部分 读懂、调试和改造

- [第 23 章 策略和版本](ch23.md)
- [第 24 章 调试 CMake](ch24.md)
- [第 25 章 旧写法为什么被取代](ch25.md)
- [第 26 章 一个库工程的完整骨架](ch26.md)

# 附录

- [附录 A 按阶段分的命令速查](appa.md)
- [附录 B 常用变量和目标属性](appb.md)
- [附录 C 副作用查看器：原理、局限和用法](appc.md)
- [附录 D 资料索引](appd.md)
