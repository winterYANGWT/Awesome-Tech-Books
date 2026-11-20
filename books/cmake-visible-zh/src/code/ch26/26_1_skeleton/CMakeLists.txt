cmake_minimum_required(VERSION 3.25...4.2)
project(ring VERSION 0.3.0 LANGUAGES CXX)

include(GenerateExportHeader)

option(RING_BUILD_TESTS "Build ring tests" ${PROJECT_IS_TOP_LEVEL})
option(RING_BUILD_EXAMPLES "Build ring examples" ${PROJECT_IS_TOP_LEVEL})
option(RING_WARNINGS "Build ring with strict warnings" ${PROJECT_IS_TOP_LEVEL})
option(RING_SANITIZE "Build ring with ASan and UBSan" OFF)

message(STATUS "ring ${PROJECT_VERSION}: top-level=${PROJECT_IS_TOP_LEVEL} tests=${RING_BUILD_TESTS} examples=${RING_BUILD_EXAMPLES} warnings=${RING_WARNINGS}")

if(RING_SANITIZE)
  add_compile_options(-fsanitize=address,undefined -fno-omit-frame-pointer)
  add_link_options(-fsanitize=address,undefined)
endif()

add_library(ring src/ring.cpp)
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

if(RING_WARNINGS)
  include(cmake/ringWarnings.cmake)
  target_link_libraries(ring PRIVATE $<BUILD_INTERFACE:ring_warnings>)
endif()

if(RING_BUILD_TESTS)
  enable_testing()
  add_subdirectory(tests)
endif()
if(RING_BUILD_EXAMPLES)
  add_subdirectory(examples)
endif()

if(PROJECT_IS_TOP_LEVEL)
  include(cmake/ringInstall.cmake)
endif()
