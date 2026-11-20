cmake_minimum_required(VERSION 3.25...4.2)
project(MissingDepends LANGUAGES CXX)

# 补上 DEPENDS：输入文件和生成脚本变了，这条命令就重跑。
add_custom_command(
  OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/table.cpp
  COMMAND ${CMAKE_COMMAND}
          -DINPUT=table.csv
          -DOUTPUT=${CMAKE_CURRENT_BINARY_DIR}/table.cpp
          -DROWS=${CMAKE_CURRENT_BINARY_DIR}/table.rows.txt
          -P gen_table.cmake
  DEPENDS table.csv gen_table.cmake
  BYPRODUCTS ${CMAKE_CURRENT_BINARY_DIR}/table.rows.txt
  WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
  VERBATIM)

add_executable(app main.cpp ${CMAKE_CURRENT_BINARY_DIR}/table.cpp)
