cmake_minimum_required(VERSION 3.25...4.2)
project(export_header LANGUAGES CXX)

include(GenerateExportHeader)
add_library(ring SHARED src/ring.cpp)
add_library(ring::ring ALIAS ring)
generate_export_header(ring
  EXPORT_FILE_NAME "${CMAKE_CURRENT_BINARY_DIR}/generated/ring/ring_export.h")
target_sources(ring
  PUBLIC FILE_SET HEADERS
    BASE_DIRS include "${CMAKE_CURRENT_BINARY_DIR}/generated"
    FILES include/ring/ring.hpp
          "${CMAKE_CURRENT_BINARY_DIR}/generated/ring/ring_export.h")
target_compile_features(ring PUBLIC cxx_std_17)
set_target_properties(ring PROPERTIES
  CXX_VISIBILITY_PRESET hidden
  VISIBILITY_INLINES_HIDDEN YES)

add_executable(app app.cpp)
target_link_libraries(app PRIVATE ring::ring)

option(BUILD_BAD "Build the app that calls the hidden function" OFF)
if(BUILD_BAD)
  add_executable(app_bad bad.cpp)
  target_link_libraries(app_bad PRIVATE ring::ring)
endif()
