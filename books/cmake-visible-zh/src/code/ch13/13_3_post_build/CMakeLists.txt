cmake_minimum_required(VERSION 3.25...4.2)
project(PostBuild LANGUAGES CXX)

add_executable(app main.cpp)

add_custom_command(TARGET app POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_CURRENT_BINARY_DIR}/dist
  COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:app> ${CMAKE_CURRENT_BINARY_DIR}/dist/app-copy
  VERBATIM)
