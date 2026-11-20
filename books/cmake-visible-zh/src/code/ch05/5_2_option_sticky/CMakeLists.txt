cmake_minimum_required(VERSION 3.25...4.2)
project(OptionSticky LANGUAGES CXX)

message(STATUS "配置前缓存 USE_FAST=[$CACHE{USE_FAST}]")
option(USE_FAST "Use the fast code path" ON)
message(STATUS "配置时 USE_FAST=${USE_FAST}")

add_executable(app main.cpp)
if(USE_FAST)
  target_compile_definitions(app PRIVATE FAST_PATH=1)
else()
  target_compile_definitions(app PRIVATE FAST_PATH=0)
endif()
