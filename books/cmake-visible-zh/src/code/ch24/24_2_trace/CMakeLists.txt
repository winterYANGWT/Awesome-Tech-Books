cmake_minimum_required(VERSION 3.25...4.2)
project(TraceDemo LANGUAGES CXX)

option(ENABLE_LOG "打开日志" ON)

set(APP_DEFS "FEATURE_FAST=1")
if(ENABLE_LOG)
  list(APPEND APP_DEFS "FEATURE_LOG=1")
endif()

set(EXTRA_FLAGS "")
if(CMAKE_BUILD_TYPE STREQUAL "Release")
  list(APPEND EXTRA_FLAGS "-O2")
endif()

add_executable(app main.cpp)
target_compile_options(app PRIVATE ${EXTRA_FLAGS})
target_compile_definitions(app PRIVATE ${APP_DEFS})

message(STATUS "APP_DEFS=${APP_DEFS}；EXTRA_FLAGS=${EXTRA_FLAGS}")
