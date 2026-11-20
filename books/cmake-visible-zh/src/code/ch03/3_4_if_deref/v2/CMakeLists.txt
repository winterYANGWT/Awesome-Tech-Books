cmake_minimum_required(VERSION 3.25...4.2)
project(IfDeref LANGUAGES CXX)

set(EMPTY "")
if(${EMPTY} STREQUAL "abc")
  message(STATUS "true")
else()
  message(STATUS "false")
endif()
