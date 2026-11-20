cmake_minimum_required(VERSION 3.25...4.2)

# 在 project() 之前写进缓存：之后 project() 才会读到它
if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR
   AND NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  set(CMAKE_BUILD_TYPE RelWithDebInfo CACHE STRING "默认构建类型")
endif()

project(BuildType LANGUAGES CXX)
add_executable(app main.cpp)
