cmake_minimum_required(VERSION 3.25...4.2)
project(DirVsTarget LANGUAGES CXX)

add_subdirectory(sub)

add_executable(app main.cpp)
target_include_directories(app PRIVATE include/common)
target_compile_definitions(app PRIVATE OWNER=1)
target_link_libraries(app PRIVATE helper)
