cmake_minimum_required(VERSION 3.25...4.2)
project(gauge VERSION 0.1.0 LANGUAGES CXX)

option(GAUGE_SANITIZE "Build with AddressSanitizer and UBSan" OFF)
if(GAUGE_SANITIZE)
  add_compile_options(-fsanitize=address,undefined -fno-omit-frame-pointer)
  add_link_options(-fsanitize=address,undefined)
endif()

add_library(gauge src/gauge.cpp)
add_library(gauge::gauge ALIAS gauge)
target_sources(gauge PUBLIC FILE_SET HEADERS
  BASE_DIRS include FILES include/gauge/gauge.hpp)
target_compile_features(gauge PUBLIC cxx_std_17)

add_executable(gauge_app app.cpp)
target_link_libraries(gauge_app PRIVATE gauge::gauge)

enable_testing()
add_executable(gauge_test tests/gauge_test.cpp)
target_link_libraries(gauge_test PRIVATE gauge::gauge)
add_test(NAME gauge COMMAND gauge_test)

message(STATUS "GAUGE_SANITIZE=${GAUGE_SANITIZE} build=${CMAKE_BUILD_TYPE}")
