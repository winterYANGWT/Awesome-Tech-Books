cmake_minimum_required(VERSION 3.25...4.2)
project(Greet VERSION 1.2.0 LANGUAGES CXX)

add_library(greet STATIC src/greet.cpp)
add_library(Greet::greet ALIAS greet)
target_include_directories(greet PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  $<INSTALL_INTERFACE:include>)

add_executable(app app/main.cpp)
target_link_libraries(app PRIVATE Greet::greet)

include(GNUInstallDirs)
include(CMakePackageConfigHelpers)
set(GREET_CMAKE_DIR ${CMAKE_INSTALL_LIBDIR}/cmake/Greet)
install(TARGETS greet EXPORT GreetTargets
  ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR})
install(EXPORT GreetTargets NAMESPACE Greet::
  DESTINATION ${GREET_CMAKE_DIR})
install(DIRECTORY include/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})

configure_package_config_file(cmake/GreetConfig.cmake.in
  "${CMAKE_CURRENT_BINARY_DIR}/GreetConfig.cmake"
  INSTALL_DESTINATION ${GREET_CMAKE_DIR})
write_basic_package_version_file(
  "${CMAKE_CURRENT_BINARY_DIR}/GreetConfigVersion.cmake"
  VERSION ${PROJECT_VERSION}
  COMPATIBILITY SameMajorVersion)
install(FILES
  "${CMAKE_CURRENT_BINARY_DIR}/GreetConfig.cmake"
  "${CMAKE_CURRENT_BINARY_DIR}/GreetConfigVersion.cmake"
  DESTINATION ${GREET_CMAKE_DIR})
