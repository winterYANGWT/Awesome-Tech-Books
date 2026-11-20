cmake_minimum_required(VERSION 3.25...4.2)
project(BuildTool LANGUAGES CXX)

add_executable(gen gen.cpp)

add_custom_command(
  OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/table.cpp
  COMMAND $<TARGET_FILE:gen>
          ${CMAKE_CURRENT_SOURCE_DIR}/table.csv
          ${CMAKE_CURRENT_BINARY_DIR}/table.cpp
  DEPENDS gen ${CMAKE_CURRENT_SOURCE_DIR}/table.csv
  VERBATIM)

add_executable(app main.cpp ${CMAKE_CURRENT_BINARY_DIR}/table.cpp)
