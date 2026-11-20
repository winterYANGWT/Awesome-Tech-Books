cmake_minimum_required(VERSION 3.25...4.2)
project(GenexString LANGUAGES CXX)

add_executable(app main.cpp)
target_compile_options(app PRIVATE "$<IF:$<CONFIG:Debug>,-O0,-O2>")

file(GENERATE OUTPUT "${CMAKE_BINARY_DIR}/genex.txt"
  CONTENT "config=$<CONFIG>\noptions=$<TARGET_PROPERTY:app,COMPILE_OPTIONS>\nbinary=$<TARGET_FILE:app>\n"
  TARGET app)
