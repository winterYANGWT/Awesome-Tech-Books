cmake_minimum_required(VERSION 3.25...4.2)
project(Default)

message(STATUS "defaults: C=${CMAKE_C_COMPILER}; CXX=${CMAKE_CXX_COMPILER}")
add_executable(app main.cpp)
