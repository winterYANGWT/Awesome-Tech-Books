cmake_minimum_required(VERSION 3.25...4.2)
project(Tiny VERSION 1.0 LANGUAGES CXX)

option(TINY_BUILD_TESTS "Build Tiny's tests" ${PROJECT_IS_TOP_LEVEL})
message(STATUS "TINY_BUILD_TESTS=${TINY_BUILD_TESTS} top=${PROJECT_IS_TOP_LEVEL}")

add_library(tiny STATIC tiny.cpp)
target_include_directories(tiny PUBLIC include)

if(TINY_BUILD_TESTS)
  add_executable(tiny_test test.cpp)
  target_link_libraries(tiny_test PRIVATE tiny)
endif()
