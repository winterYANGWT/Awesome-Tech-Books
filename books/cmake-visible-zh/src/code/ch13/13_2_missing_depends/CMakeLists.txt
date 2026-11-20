cmake_minimum_required(VERSION 3.25...4.2)
project(MissingDepends LANGUAGES CXX)

# 反例：只声明了命令和输出，没有声明它读过 table.csv。
add_custom_command(
  OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/table.cpp
  COMMAND ${CMAKE_COMMAND}
          -DINPUT=table.csv
          -DOUTPUT=${CMAKE_CURRENT_BINARY_DIR}/table.cpp
          -DROWS=${CMAKE_CURRENT_BINARY_DIR}/table.rows.txt
          -P gen_table.cmake
  BYPRODUCTS ${CMAKE_CURRENT_BINARY_DIR}/table.rows.txt
  WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
  VERBATIM)

add_executable(app main.cpp ${CMAKE_CURRENT_BINARY_DIR}/table.cpp)
