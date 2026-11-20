cmake_minimum_required(VERSION 3.25...4.2)
project(Launcher LANGUAGES CXX)

add_executable(app main.cpp greet.cpp)
target_compile_features(app PRIVATE cxx_std_17)
