cmake_minimum_required(VERSION 3.25...4.2)
project(Consumer LANGUAGES CXX)

include(FetchContent)
FetchContent_Declare(tiny SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../tiny)
FetchContent_MakeAvailable(tiny)

add_executable(app main.cpp)
target_link_libraries(app PRIVATE tiny)
