cmake_minimum_required(VERSION 3.25...4.2)
project(ThreeKeywords LANGUAGES CXX)

add_library(calc STATIC calc.cpp)
target_compile_definitions(calc PRIVATE CALC_PRIVATE=1)
target_compile_definitions(calc INTERFACE CALC_INTERFACE=2)
target_compile_definitions(calc PUBLIC CALC_PUBLIC=3)

add_executable(app main.cpp)
target_link_libraries(app PRIVATE calc)
