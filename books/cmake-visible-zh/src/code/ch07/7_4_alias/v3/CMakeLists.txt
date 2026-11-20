cmake_minimum_required(VERSION 3.25...4.2)
project(Alias LANGUAGES CXX)

add_library(greet greet.cpp)
target_include_directories(greet PUBLIC include)
add_library(acme::greet ALIAS greet)

add_executable(app main.cpp)
target_link_libraries(app PRIVATE gret)
