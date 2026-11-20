cmake_minimum_required(VERSION 3.25...4.2)
project(Probe LANGUAGES CXX)

message(STATUS "compiler=${CMAKE_CXX_COMPILER}; ar=${CMAKE_AR}; build_type=[${CMAKE_BUILD_TYPE}]; prefix=${CMAKE_INSTALL_PREFIX}")

add_executable(app main.cpp)
