cmake_minimum_required(VERSION 3.25...4.2)
project(Rerun LANGUAGES CXX)

add_executable(greet main.cpp)
target_compile_definitions(greet PRIVATE GREETING="hi")
