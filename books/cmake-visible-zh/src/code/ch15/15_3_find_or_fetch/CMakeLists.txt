cmake_minimum_required(VERSION 3.25...4.2)
project(FindOrFetch LANGUAGES CXX)

include(FetchContent)
FetchContent_Declare(GTest
  URL ${CMAKE_CURRENT_SOURCE_DIR}/deps/fake-gtest-1.0.tar.gz
  URL_HASH SHA256=ac005653ae7c88b01aa6b8c03a830687dd88494bba87dbf81ad697e543ef1ef4
  FIND_PACKAGE_ARGS NAMES GTest CONFIG
)
FetchContent_MakeAvailable(GTest)

if(EXISTS "${CMAKE_BINARY_DIR}/_deps")
  message(STATUS "build/_deps 存在")
else()
  message(STATUS "build/_deps 不存在")
endif()

add_executable(app main.cpp)
target_link_libraries(app PRIVATE GTest::gtest_main)
