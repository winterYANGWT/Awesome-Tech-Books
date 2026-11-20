cmake_minimum_required(VERSION 3.25...4.2)
project(Toolchain LANGUAGES CXX)

message(STATUS "project() 之后：${CMAKE_CXX_COMPILER}（${CMAKE_CXX_COMPILER_ID} ${CMAKE_CXX_COMPILER_VERSION}）")
message(STATUS "缓存里的工具链文件：$CACHE{CMAKE_TOOLCHAIN_FILE}")
message(STATUS "project() 准备执行：${INCLUDE_CMAKE_TOOLCHAIN_FILE_IF_REQUIRED}")

add_library(greet greet.cpp)
add_executable(app main.cpp)
target_link_libraries(app PRIVATE greet)
