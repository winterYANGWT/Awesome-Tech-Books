cmake_minimum_required(VERSION 3.25...4.2)
project(Host LANGUAGES CXX)

add_subdirectory(../lib lib)

add_executable(host_app main.cpp)
target_link_libraries(host_app PRIVATE calc)
