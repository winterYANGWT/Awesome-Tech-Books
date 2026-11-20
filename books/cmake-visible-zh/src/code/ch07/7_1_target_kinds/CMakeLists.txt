cmake_minimum_required(VERSION 3.25...4.2)
project(TargetKinds LANGUAGES CXX)

add_library(stat STATIC stat.cpp)
add_library(shared SHARED shared.cpp)
add_library(obj OBJECT obj.cpp)
add_library(iface INTERFACE)
target_include_directories(iface INTERFACE include)

add_executable(app main.cpp)
target_link_libraries(app PRIVATE stat shared obj iface)

add_custom_target(notes ALL
  COMMAND ${CMAKE_COMMAND} -E echo "notes: 自定义目标没有产物"
  VERBATIM)
