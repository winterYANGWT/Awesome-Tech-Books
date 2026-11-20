cmake_minimum_required(VERSION 3.25...4.2)
project(CustomOutput LANGUAGES CXX)

add_custom_command(
  OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/table.cpp
  COMMAND ${CMAKE_COMMAND}
          -DINPUT=${CMAKE_CURRENT_SOURCE_DIR}/table.csv
          -DOUTPUT=${CMAKE_CURRENT_BINARY_DIR}/table.cpp
          -P ${CMAKE_CURRENT_SOURCE_DIR}/gen_table.cmake
  DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/table.csv
          ${CMAKE_CURRENT_SOURCE_DIR}/gen_table.cmake
  VERBATIM)

add_executable(app main.cpp ${CMAKE_CURRENT_BINARY_DIR}/table.cpp)
