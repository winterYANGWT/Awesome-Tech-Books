cmake_minimum_required(VERSION 3.25...4.2)
project(PolicyWarning LANGUAGES CXX)

add_library(core STATIC core.cpp)
add_executable(app main.cpp)
target_link_libraries(app PRIVATE core)
