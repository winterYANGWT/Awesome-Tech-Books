cmake_minimum_required(VERSION 3.25...4.2)
project(Tiny LANGUAGES CXX)

option(TINY_BUILD_TOOL "Build the tiny command line tool" ON)
message(STATUS "tiny: TINY_BUILD_TOOL=${TINY_BUILD_TOOL}")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall")

add_library(tiny STATIC tiny.cpp)
target_include_directories(tiny PUBLIC include)

if(TINY_BUILD_TOOL)
  add_executable(tiny_tool tool.cpp)
  target_link_libraries(tiny_tool PRIVATE tiny)
endif()
