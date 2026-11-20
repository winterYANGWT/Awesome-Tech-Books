cmake_minimum_required(VERSION 3.25...4.2)
project(MacroLeak LANGUAGES CXX)

macro(make_variant name)
  set(tmp "${name}_internal")
  add_executable(${name} main.cpp)
  target_compile_definitions(${name} PRIVATE MODE="${tmp}" LABEL="${name}")
endmacro()

set(tmp "user-visible")
make_variant(tool)
message(STATUS "after call tmp=${tmp}")

add_executable(app main.cpp)
target_compile_definitions(app PRIVATE MODE="${tmp}" LABEL="app")
