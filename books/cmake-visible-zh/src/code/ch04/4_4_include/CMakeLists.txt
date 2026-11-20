cmake_minimum_required(VERSION 3.25...4.2)
project(IncludeDemo LANGUAGES CXX)

include(cmake/helpers.cmake)

message(STATUS "顶层: HELPER_GREETING=${HELPER_GREETING}")
message(STATUS "顶层: CMAKE_CURRENT_LIST_DIR=${CMAKE_CURRENT_LIST_DIR}")

add_executable(app main.cpp)
add_greeting(app)
