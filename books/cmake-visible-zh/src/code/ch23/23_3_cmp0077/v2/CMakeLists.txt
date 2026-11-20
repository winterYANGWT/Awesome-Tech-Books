cmake_minimum_required(VERSION 3.25...4.2)
project(OptionPolicy LANGUAGES CXX)

cmake_policy(SET CMP0077 OLD)

set(WANT_FAST OFF)
option(WANT_FAST "build the fast variant" ON)

if(DEFINED CACHE{WANT_FAST})
  message(STATUS "cache: WANT_FAST=$CACHE{WANT_FAST}")
else()
  message(STATUS "cache: 没有 WANT_FAST")
endif()
message(STATUS "WANT_FAST=${WANT_FAST}")

configure_file(config.h.in config.h @ONLY)
add_executable(app main.cpp)
target_include_directories(app PRIVATE ${CMAKE_CURRENT_BINARY_DIR})
