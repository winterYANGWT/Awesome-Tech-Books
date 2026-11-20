cmake_minimum_required(VERSION 3.25...4.2)
project(Expansion LANGUAGES CXX)

set(var_name "FLAVOR")
set(FLAVOR "debug")
message(STATUS "直接引用 ${FLAVOR}")
message(STATUS "嵌套引用 ${${var_name}}")
message(STATUS "未定义变量 [${NOT_SET}]")

set(BOOK_STAGE "configure" CACHE STRING "演示 $CACHE{}")
message(STATUS "缓存变量 $CACHE{BOOK_STAGE}")
message(STATUS "普通变量不在缓存里 [$CACHE{FLAVOR}]")
message(STATUS "环境变量 [$ENV{BOOK_FLAVOR}]")

configure_file(stage.h.in "${CMAKE_CURRENT_BINARY_DIR}/stage.h" @ONLY)
add_executable(app main.cpp)
target_include_directories(app PRIVATE "${CMAKE_CURRENT_BINARY_DIR}")
target_compile_definitions(app PRIVATE "BUILD_NAME=\"${PROJECT_NAME}-${FLAVOR}\"")
