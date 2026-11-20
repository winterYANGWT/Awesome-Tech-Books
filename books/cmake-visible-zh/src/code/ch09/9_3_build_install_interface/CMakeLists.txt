cmake_minimum_required(VERSION 3.25...4.2)
project(Interfaces LANGUAGES CXX)

add_library(greet STATIC greet.cpp)
target_include_directories(greet PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  $<INSTALL_INTERFACE:include>)

add_executable(app main.cpp)
target_link_libraries(app PRIVATE greet)

include(GNUInstallDirs)
install(TARGETS greet EXPORT GreetTargets
  ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR})
install(EXPORT GreetTargets FILE GreetTargets.cmake
  DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/Greet)
install(DIRECTORY include/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
