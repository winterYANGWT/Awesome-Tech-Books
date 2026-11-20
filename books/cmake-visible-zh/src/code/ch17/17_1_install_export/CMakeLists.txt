cmake_minimum_required(VERSION 3.25...4.2)
project(Greet LANGUAGES CXX)

add_library(greet STATIC src/greet.cpp)
add_library(Greet::greet ALIAS greet)
target_include_directories(greet PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  $<INSTALL_INTERFACE:include>)

add_executable(app app/main.cpp)
target_link_libraries(app PRIVATE Greet::greet)

include(GNUInstallDirs)
install(TARGETS greet EXPORT GreetTargets
  ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR})
install(EXPORT GreetTargets
  NAMESPACE Greet::
  DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/Greet)
install(DIRECTORY include/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
