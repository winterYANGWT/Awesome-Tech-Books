cmake_minimum_required(VERSION 3.25...4.2)
project(FindModule LANGUAGES CXX)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
find_package(Foo REQUIRED)
message(STATUS "Foo_INCLUDE_DIR=${Foo_INCLUDE_DIR}")
message(STATUS "Foo_LIBRARY=${Foo_LIBRARY}")

add_executable(app main.cpp)
target_link_libraries(app PRIVATE Foo::Foo)
