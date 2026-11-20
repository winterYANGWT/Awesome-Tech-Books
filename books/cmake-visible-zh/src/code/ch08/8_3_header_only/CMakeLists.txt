cmake_minimum_required(VERSION 3.25...4.2)
project(HeaderOnly LANGUAGES CXX)

add_library(units INTERFACE)
target_include_directories(units INTERFACE include)
target_compile_features(units INTERFACE cxx_std_17)
target_compile_definitions(units INTERFACE UNITS_LEVEL=2)

add_executable(app main.cpp)
target_link_libraries(app PRIVATE units)
