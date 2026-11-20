cmake_minimum_required(VERSION 3.25...4.2)
project(ParentOverride LANGUAGES CXX)

set(SUB_BUILD_TESTS OFF)
add_subdirectory(sub)

add_executable(app main.cpp)
target_link_libraries(app PRIVATE sub)
