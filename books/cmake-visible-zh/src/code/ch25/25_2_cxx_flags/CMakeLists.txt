cmake_minimum_required(VERSION 3.25...4.2)
project(FlagsLeak LANGUAGES CXX)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Werror")

add_subdirectory(vendor)
add_executable(app main.cpp)
target_link_libraries(app PRIVATE legacy)
