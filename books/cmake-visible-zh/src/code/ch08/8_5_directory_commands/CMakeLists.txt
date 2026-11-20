cmake_minimum_required(VERSION 3.25...4.2)
project(DirectoryCommands LANGUAGES CXX)

add_library(extra STATIC extra.cpp)
target_include_directories(extra PUBLIC extra_include)

add_executable(early early.cpp)

include_directories(include)
add_compile_options(-DOPT_SEEN=1)
add_definitions(-DDEF_SEEN=1)
link_libraries(extra)

add_executable(late late.cpp)
add_subdirectory(sub)
