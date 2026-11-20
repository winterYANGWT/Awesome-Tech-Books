cmake_minimum_required(VERSION 3.25...4.2)
project(ConfigurePreset LANGUAGES CXX)

message(STATUS "preset 给的值：config=${CMAKE_BUILD_TYPE} level=${PROJ_OPT_LEVEL} warn=${PROJ_WARNINGS}")

add_executable(app main.cpp)
target_compile_definitions(app PRIVATE
  PROJ_CONFIG="${CMAKE_BUILD_TYPE}"
  PROJ_OPT_LEVEL=${PROJ_OPT_LEVEL})

if(PROJ_WARNINGS)
  target_compile_options(app PRIVATE -Wall -Wextra)
endif()
