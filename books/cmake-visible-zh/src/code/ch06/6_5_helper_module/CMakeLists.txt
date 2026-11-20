cmake_minimum_required(VERSION 3.25...4.2)
project(HelperModule VERSION 0.9 LANGUAGES CXX)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
include(MyHelpers)

myhelpers_configure_banner(BANNER_HEADER BANNER_INCLUDE_DIR)
myhelpers_make_identifier(BANNER_ID "banner-${PROJECT_VERSION}")

message(STATUS "头文件=${BANNER_HEADER}")
message(STATUS "包含目录=${BANNER_INCLUDE_DIR}  标识=${BANNER_ID}")

add_executable(app main.cpp)
target_include_directories(app PRIVATE "${BANNER_INCLUDE_DIR}")
target_compile_definitions(app PRIVATE BANNER_ID="${BANNER_ID}")
