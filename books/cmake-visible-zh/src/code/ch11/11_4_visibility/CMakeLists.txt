cmake_minimum_required(VERSION 3.25...4.2)
project(Visibility LANGUAGES CXX)

include(GenerateExportHeader)

add_library(x SHARED src/x.cpp)
target_include_directories(x PUBLIC include ${CMAKE_CURRENT_BINARY_DIR})
set_target_properties(x PROPERTIES
  CXX_VISIBILITY_PRESET hidden
  VISIBILITY_INLINES_HIDDEN ON)
generate_export_header(x)

add_executable(app main.cpp)
target_link_libraries(app PRIVATE x)
