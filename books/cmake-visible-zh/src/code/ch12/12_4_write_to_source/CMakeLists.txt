cmake_minimum_required(VERSION 3.25...4.2)
project(WriteToSource LANGUAGES CXX)

set(VARIANT "team-a" CACHE STRING "写进 version.h 的团队名")
configure_file(version.h.in ${CMAKE_CURRENT_SOURCE_DIR}/version.h)

file(WRITE relative.txt "file(WRITE) 的相对路径落在源码目录\n")

add_executable(app main.cpp)
target_include_directories(app PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
