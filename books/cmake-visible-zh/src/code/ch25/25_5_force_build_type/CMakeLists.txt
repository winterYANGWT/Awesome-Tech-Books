cmake_minimum_required(VERSION 3.25...4.2)
project(ForceBuildType LANGUAGES CXX)

set(CMAKE_BUILD_TYPE Release)
message(STATUS "build type: ${CMAKE_BUILD_TYPE}")
message(STATUS "cache: $CACHE{CMAKE_BUILD_TYPE}")

add_executable(app main.cpp)
