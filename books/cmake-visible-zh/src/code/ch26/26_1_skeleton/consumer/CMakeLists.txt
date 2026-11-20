cmake_minimum_required(VERSION 3.25...4.2)
project(consumer LANGUAGES CXX)

find_package(ring 0.3 CONFIG REQUIRED)
message(STATUS "found ring ${ring_VERSION} at ${ring_DIR}")

add_executable(consumer main.cpp)
target_link_libraries(consumer PRIVATE ring::ring)
