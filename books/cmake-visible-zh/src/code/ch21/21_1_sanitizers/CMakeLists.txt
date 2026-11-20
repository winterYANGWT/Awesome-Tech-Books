cmake_minimum_required(VERSION 3.25...4.2)
project(Sanitize LANGUAGES CXX)

set(PROJ_SANITIZE "" CACHE STRING "address;undefined 或 thread；thread 可和 undefined 同用")

include(cmake/Sanitizers.cmake)

add_executable(probe main.cpp)
target_compile_features(probe PRIVATE cxx_std_17)
proj_enable_sanitizers(probe)
