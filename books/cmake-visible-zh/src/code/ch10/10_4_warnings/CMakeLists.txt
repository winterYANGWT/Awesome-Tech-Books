cmake_minimum_required(VERSION 3.25...4.2)
project(Warnings LANGUAGES CXX)

file(WRITE "${CMAKE_BINARY_DIR}/compiler.txt"
  "id=${CMAKE_CXX_COMPILER_ID}\nfrontend=${CMAKE_CXX_COMPILER_FRONTEND_VARIANT}\nsimulate=${CMAKE_CXX_SIMULATE_ID}\n")

add_library(proj_warnings INTERFACE)
target_compile_options(proj_warnings INTERFACE
  $<$<CXX_COMPILER_ID:MSVC>:/W4>
  $<$<NOT:$<CXX_COMPILER_ID:MSVC>>:-Wall -Wextra -Wpedantic>)

add_executable(app main.cpp)
target_link_libraries(app PRIVATE proj_warnings)

add_executable(tool tool.cpp)
