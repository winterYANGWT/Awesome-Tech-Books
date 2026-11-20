cmake_minimum_required(VERSION 3.25...4.2)
project(CrossAarch64 LANGUAGES CXX)

message(STATUS "cross=${CMAKE_CROSSCOMPILING} system=${CMAKE_SYSTEM_NAME} proc=${CMAKE_SYSTEM_PROCESSOR}")
message(STATUS "clang target=${CMAKE_CXX_COMPILER_TARGET} sysroot='${CMAKE_SYSROOT}'")
message(STATUS "find 模式：program=${CMAKE_FIND_ROOT_PATH_MODE_PROGRAM} library=${CMAKE_FIND_ROOT_PATH_MODE_LIBRARY} include=${CMAKE_FIND_ROOT_PATH_MODE_INCLUDE}")

add_library(blob STATIC blob.cpp)
