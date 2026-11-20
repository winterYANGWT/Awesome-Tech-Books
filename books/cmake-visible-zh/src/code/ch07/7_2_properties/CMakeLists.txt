cmake_minimum_required(VERSION 3.25...4.2)
project(Properties LANGUAGES CXX)

add_executable(app main.cpp)
target_compile_definitions(app PRIVATE MODE=1)

set_target_properties(app PROPERTIES
  OUTPUT_NAME demo
  RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")

get_target_property(app_out app OUTPUT_NAME)
message(STATUS "get_target_property：OUTPUT_NAME=${app_out}")

set_property(TARGET app APPEND PROPERTY COMPILE_DEFINITIONS EXTRA=2)
