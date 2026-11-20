cmake_minimum_required(VERSION 3.25...4.2)
project(DirScope LANGUAGES CXX)

set(LEVEL "top")
message(STATUS "顶层设置 LEVEL=${LEVEL}")

add_subdirectory(sub)

message(STATUS "回到顶层 LEVEL=${LEVEL}")
message(STATUS "子目录写回的 FROM_SUB=${FROM_SUB}")

add_executable(app main.cpp)
target_compile_definitions(app PRIVATE
  "LEVEL=\"${LEVEL}\""
  "FROM_SUB=\"${FROM_SUB}\"")
