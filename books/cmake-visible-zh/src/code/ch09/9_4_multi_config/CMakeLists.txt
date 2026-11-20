cmake_minimum_required(VERSION 3.25...4.2)
project(MultiConfig LANGUAGES CXX)

message(STATUS "可用的配置：${CMAKE_CONFIGURATION_TYPES}；CMAKE_BUILD_TYPE=[${CMAKE_BUILD_TYPE}]")

add_executable(app main.cpp)
target_compile_definitions(app PRIVATE "APP_CONFIG=\"$<CONFIG>\"")
file(GENERATE OUTPUT "${CMAKE_BINARY_DIR}/config-$<CONFIG>.txt"
  CONTENT "config=$<CONFIG>\n")
