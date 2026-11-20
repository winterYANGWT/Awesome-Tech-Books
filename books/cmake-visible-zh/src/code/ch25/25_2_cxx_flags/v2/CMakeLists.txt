cmake_minimum_required(VERSION 3.25...4.2)
project(FlagsLeak LANGUAGES CXX)

add_subdirectory(vendor)
add_executable(app main.cpp)
target_compile_options(app PRIVATE -Wall -Werror)
target_link_libraries(app PRIVATE legacy)
