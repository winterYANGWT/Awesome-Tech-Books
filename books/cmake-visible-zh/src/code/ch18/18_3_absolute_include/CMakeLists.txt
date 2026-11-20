cmake_minimum_required(VERSION 3.25...4.2)
project(Greet VERSION 1.2.0 LANGUAGES CXX)

add_library(greet STATIC src/greet.cpp)
target_include_directories(greet PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  ${CMAKE_CURRENT_BINARY_DIR}/generated)

include(GNUInstallDirs)
install(TARGETS greet EXPORT GreetTargets
  ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR})
install(EXPORT GreetTargets NAMESPACE Greet::
  DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/Greet)
