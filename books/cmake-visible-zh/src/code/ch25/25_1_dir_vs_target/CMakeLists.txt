cmake_minimum_required(VERSION 3.25...4.2)
project(DirVsTarget LANGUAGES CXX)

include_directories(include/common)
add_definitions(-DOWNER=1)

add_subdirectory(sub)

add_executable(app main.cpp)
target_link_libraries(app PRIVATE helper)
