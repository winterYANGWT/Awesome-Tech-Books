cmake_minimum_required(VERSION 3.25)
project(PolicyRange LANGUAGES CXX)

# GET 读的是当前状态，不是某个常量
cmake_policy(GET CMP0156 cmp0156)
cmake_policy(GET CMP0181 cmp0181)
cmake_policy(GET CMP0077 cmp0077)
foreach(p cmp0156 cmp0181 cmp0077)
  if(NOT ${p})
    set(${p} "unset")
  endif()
endforeach()
message(STATUS "CMP0156=${cmp0156} CMP0181=${cmp0181} CMP0077=${cmp0077}")

configure_file(policy.h.in policy.h @ONLY)
add_executable(policy_demo main.cpp)
target_include_directories(policy_demo PRIVATE ${CMAKE_CURRENT_BINARY_DIR})
