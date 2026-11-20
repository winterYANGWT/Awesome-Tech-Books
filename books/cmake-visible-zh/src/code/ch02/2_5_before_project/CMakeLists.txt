cmake_minimum_required(VERSION 3.25...4.2)
project(Switch LANGUAGES CXX)

message(STATUS "compiler=${CMAKE_CXX_COMPILER}; id=${CMAKE_CXX_COMPILER_ID}")
add_executable(app main.cpp)
