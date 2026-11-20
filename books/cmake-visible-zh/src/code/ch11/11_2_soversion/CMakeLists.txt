cmake_minimum_required(VERSION 3.25...4.2)
project(Soversion LANGUAGES CXX)

add_library(x SHARED src.cpp)
target_include_directories(x PUBLIC include)
set_target_properties(x PROPERTIES
  VERSION 1.2.3
  SOVERSION 1)

add_executable(app main.cpp)
target_link_libraries(app PRIVATE x)
