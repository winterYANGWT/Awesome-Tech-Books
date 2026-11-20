# 工具链文件：用 --toolchain cmake/clang.cmake 指定
# message 只为看清它被读了几次；正式工程里别在这里写有副作用的命令
message(STATUS "读 clang.cmake：此刻 CMAKE_CXX_COMPILER='${CMAKE_CXX_COMPILER}'")
set(CMAKE_CXX_COMPILER clang++)
