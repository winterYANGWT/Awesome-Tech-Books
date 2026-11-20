cmake_minimum_required(VERSION 3.25...4.2)
project(ObjectLib LANGUAGES CXX)

add_library(common OBJECT common.cpp)
target_include_directories(common PUBLIC include)

add_executable(app main.cpp)
target_link_libraries(app PRIVATE common)

add_executable(tool tool.cpp)
target_link_libraries(tool PRIVATE common)
