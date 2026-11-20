cmake_minimum_required(VERSION 3.25...4.2)
project(MessageLevels LANGUAGES CXX)

list(APPEND CMAKE_MESSAGE_CONTEXT "demo")

option(USE_FAST "用快速实现" ON)
set(IMPL "slow")

list(APPEND CMAKE_MESSAGE_CONTEXT "scan")
message(CHECK_START "查找 fast 实现")
if(USE_FAST AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/fast.cpp")
  message(CHECK_PASS "找到 fast.cpp")
  set(IMPL "fast")
else()
  message(CHECK_FAIL "没有 fast.cpp，退回 slow")
endif()

message(CHECK_START "检查构建类型")
if(CMAKE_BUILD_TYPE)
  message(CHECK_PASS "${CMAKE_BUILD_TYPE}")
else()
  message(CHECK_FAIL "未指定，使用编译器默认值")
endif()
list(POP_BACK CMAKE_MESSAGE_CONTEXT)

message(STATUS "选用的实现：${IMPL}")
message(VERBOSE "编译定义：APP_IMPL_${IMPL}=1")
message(DEBUG "缓存里的 USE_FAST=${USE_FAST}，构建目录=${CMAKE_BINARY_DIR}")
message(TRACE "源文件列表：main.cpp;${IMPL}.cpp")

configure_file(config.hpp.in "${CMAKE_CURRENT_BINARY_DIR}/config.hpp" @ONLY)

add_executable(app main.cpp ${IMPL}.cpp)
target_include_directories(app PRIVATE "${CMAKE_CURRENT_BINARY_DIR}")
target_compile_features(app PRIVATE cxx_std_17)
target_compile_definitions(app PRIVATE "APP_IMPL_${IMPL}=1")
