cmake_minimum_required(VERSION 3.25...4.2)
project(BuildSharedLibs LANGUAGES CXX)

add_library(x src.cpp)
target_include_directories(x PUBLIC include)

add_executable(app main.cpp)
target_link_libraries(app PRIVATE x)
