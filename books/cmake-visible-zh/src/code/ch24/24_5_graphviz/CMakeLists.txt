cmake_minimum_required(VERSION 3.25...4.2)
project(Graph LANGUAGES CXX)

find_package(ZLIB REQUIRED)

add_library(util STATIC util.cpp)
add_library(core STATIC core.cpp)
target_link_libraries(core PUBLIC util)
target_link_libraries(core PRIVATE ZLIB::ZLIB)

add_executable(app main.cpp)
target_link_libraries(app PRIVATE core)
