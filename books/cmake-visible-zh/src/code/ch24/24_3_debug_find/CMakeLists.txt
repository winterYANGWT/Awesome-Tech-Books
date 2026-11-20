cmake_minimum_required(VERSION 3.25...4.2)
project(DebugFind LANGUAGES CXX)

option(BUILD_TESTS "构建测试" OFF)

# 只为让调试输出短一些：不搜索 PATH 上的每个目录
set(CMAKE_FIND_USE_SYSTEM_ENVIRONMENT_PATH OFF)

find_package(ZLIB REQUIRED)

add_executable(app main.cpp)
target_link_libraries(app PRIVATE ZLIB::ZLIB)
target_compile_features(app PRIVATE cxx_std_17)

if(BUILD_TESTS)
  find_package(GTest CONFIG REQUIRED)
  enable_testing()
  add_executable(check check.cpp)
  target_link_libraries(check PRIVATE GTest::gtest_main ZLIB::ZLIB)
  add_test(NAME check COMMAND check)
endif()
