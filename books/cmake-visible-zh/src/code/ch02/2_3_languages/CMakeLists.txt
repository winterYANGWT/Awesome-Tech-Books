cmake_minimum_required(VERSION 3.25...4.2)
project(NoLang LANGUAGES NONE)

message(STATUS "before enable: CXX=[${CMAKE_CXX_COMPILER}]")

if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
  enable_language(CXX)
endif()

message(STATUS "after enable: CXX=${CMAKE_CXX_COMPILER}")
add_executable(app main.cpp)
