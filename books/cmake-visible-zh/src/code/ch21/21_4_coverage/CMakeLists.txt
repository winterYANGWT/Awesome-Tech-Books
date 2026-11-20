cmake_minimum_required(VERSION 3.25...4.2)

# 这两个缓存项要在 project() 之前给默认值；project() 会为当前构建类型建空的缓存项（标准配置会写入默认值）
set(CMAKE_CXX_FLAGS_COVERAGE "--coverage -O0 -g" CACHE STRING "Coverage 构建的编译参数")
set(CMAKE_EXE_LINKER_FLAGS_COVERAGE "--coverage" CACHE STRING "Coverage 构建的链接参数")

project(Coverage LANGUAGES CXX)

add_executable(check main.cpp)
target_compile_features(check PRIVATE cxx_std_17)

enable_testing()
add_test(NAME sum COMMAND check)
