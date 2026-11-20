cmake_minimum_required(VERSION 3.25...4.2)
project(ExecuteProcess LANGUAGES CXX)
execute_process(
  COMMAND ${CMAKE_CXX_COMPILER} --version
  OUTPUT_VARIABLE compiler_version
  RESULT_VARIABLE compiler_result
  OUTPUT_STRIP_TRAILING_WHITESPACE)
if(NOT compiler_result EQUAL 0)
  message(FATAL_ERROR "查询编译器版本失败：${compiler_result}")
endif()
string(REGEX MATCH "^[^\n]*" compiler_line "${compiler_version}")

execute_process(
  COMMAND ${CMAKE_COMMAND} -E cat ${CMAKE_CURRENT_SOURCE_DIR}/tool-version.txt
  OUTPUT_VARIABLE tool_version
  RESULT_VARIABLE tool_result
  OUTPUT_STRIP_TRAILING_WHITESPACE)
if(NOT tool_result EQUAL 0)
  message(FATAL_ERROR "读取 tool-version.txt 失败：${tool_result}")
endif()
set(COMPILER_LINE "${compiler_line}")
set(TOOL_VERSION "${tool_version}")
configure_file(version.h.in ${CMAKE_CURRENT_BINARY_DIR}/version.h)
add_executable(app main.cpp)
target_include_directories(app PRIVATE ${CMAKE_CURRENT_BINARY_DIR})
