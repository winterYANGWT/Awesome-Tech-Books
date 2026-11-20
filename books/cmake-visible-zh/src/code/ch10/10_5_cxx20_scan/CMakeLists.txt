cmake_minimum_required(VERSION 3.25...4.2)
project(Scan LANGUAGES CXX)

add_executable(app main.cpp)
target_compile_features(app PRIVATE cxx_std_20)

add_executable(plain plain.cpp)
