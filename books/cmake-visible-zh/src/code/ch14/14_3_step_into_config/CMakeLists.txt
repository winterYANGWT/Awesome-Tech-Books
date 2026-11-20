cmake_minimum_required(VERSION 3.25...4.2)
project(StepInto LANGUAGES CXX)

list(APPEND CMAKE_PREFIX_PATH "${CMAKE_CURRENT_SOURCE_DIR}/deps/prefix")

find_package(Tiny 1.2 CONFIG REQUIRED)
get_target_property(tiny_includes Tiny::tiny INTERFACE_INCLUDE_DIRECTORIES)
message(STATUS "Tiny_VERSION=${Tiny_VERSION} Tiny_DIR=${Tiny_DIR}")
message(STATUS "Tiny_INCLUDE=${tiny_includes}")

add_executable(app main.cpp)
target_link_libraries(app PRIVATE Tiny::tiny)
