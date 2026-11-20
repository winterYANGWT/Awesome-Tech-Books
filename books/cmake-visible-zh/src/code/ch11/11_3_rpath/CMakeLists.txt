cmake_minimum_required(VERSION 3.25...4.2)
project(Rpath LANGUAGES CXX)

add_library(x SHARED src.cpp)
target_include_directories(x PUBLIC include)

add_executable(app main.cpp)
target_link_libraries(app PRIVATE x)
