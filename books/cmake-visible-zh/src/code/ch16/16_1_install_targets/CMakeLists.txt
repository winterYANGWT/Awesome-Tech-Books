cmake_minimum_required(VERSION 3.25...4.2)
project(InstallTargets LANGUAGES CXX)

add_library(greet STATIC greet.cpp)
target_sources(greet PUBLIC
  FILE_SET HEADERS BASE_DIRS include FILES include/greet/greet.h)

add_executable(greet_app main.cpp)
target_link_libraries(greet_app PRIVATE greet)

install(TARGETS greet greet_app
  ARCHIVE DESTINATION lib
  RUNTIME DESTINATION bin
  FILE_SET HEADERS DESTINATION include)
