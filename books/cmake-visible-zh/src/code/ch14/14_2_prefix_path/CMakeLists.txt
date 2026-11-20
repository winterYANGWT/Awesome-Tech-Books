cmake_minimum_required(VERSION 3.25...4.2)
project(PrefixPath LANGUAGES CXX)

find_package(Tiny CONFIG REQUIRED)
message(STATUS "Tiny_VERSION=${Tiny_VERSION}")
message(STATUS "Tiny_DIR=${Tiny_DIR}")
message(STATUS "Tiny_CONFIG=${Tiny_CONFIG}")

add_executable(app main.cpp)
target_link_libraries(app PRIVATE Tiny::tiny)
