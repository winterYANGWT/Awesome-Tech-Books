cmake_minimum_required(VERSION 3.25...4.2)
project(FetchTiny LANGUAGES CXX)

include(FetchContent)
FetchContent_Declare(tiny
  URL ${CMAKE_CURRENT_SOURCE_DIR}/deps/tiny-1.0.tar.gz
  URL_HASH SHA256=3fc44f6b5e8cd46a005a0f0be3e0abe3ae148ac303848996fa9a12605b203bbe
)
if(EXISTS "${CMAKE_BINARY_DIR}/_deps")
  message(STATUS "配置前：_deps 已存在")
else()
  message(STATUS "配置前：没有 _deps")
endif()
FetchContent_MakeAvailable(tiny)
message(STATUS "tiny 源码在 ${tiny_SOURCE_DIR}")

add_executable(app main.cpp)
target_link_libraries(app PRIVATE tiny)
