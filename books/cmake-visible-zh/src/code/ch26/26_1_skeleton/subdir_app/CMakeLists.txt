cmake_minimum_required(VERSION 3.25...4.2)
project(subdir_app LANGUAGES CXX)

add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/.." ring-build)

message(STATUS "subdir app: ring tests=${RING_BUILD_TESTS} examples=${RING_BUILD_EXAMPLES}")

add_executable(subdir_app main.cpp)
target_link_libraries(subdir_app PRIVATE ring::ring)
