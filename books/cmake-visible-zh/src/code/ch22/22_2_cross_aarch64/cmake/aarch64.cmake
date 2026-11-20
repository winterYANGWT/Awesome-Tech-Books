# 交叉编译工具链：目标 Linux / aarch64，编译器用 clang
set(CMAKE_SYSTEM_NAME Linux)
set(CMAKE_SYSTEM_VERSION 1)
set(CMAKE_SYSTEM_PROCESSOR aarch64)

set(CMAKE_CXX_COMPILER clang++)
set(CMAKE_CXX_COMPILER_TARGET aarch64-linux-gnu)

# 本机没有 aarch64 的 sysroot：检测编译器时只编译、不链接
set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)

# find_library()/find_path()/find_package() 只在目标根下找；
# find_program() 找的是主机上运行的工具，不能在目标根下找
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ONLY)
