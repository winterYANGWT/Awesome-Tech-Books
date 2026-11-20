cmake_minimum_required(VERSION 3.25...4.2)
project(Calc LANGUAGES CXX)

add_library(calc STATIC calc.cpp)
target_include_directories(calc PUBLIC include)

if(PROJECT_IS_TOP_LEVEL)
  include(CTest)
endif()

if(PROJECT_IS_TOP_LEVEL AND BUILD_TESTING)
  add_subdirectory(tests)
endif()
