cmake_minimum_required(VERSION 3.25...4.2)
project(TwoModes LANGUAGES CXX)

find_package(ZLIB REQUIRED)
find_package(GTest CONFIG REQUIRED)

message(STATUS "ZLIB_VERSION=${ZLIB_VERSION} ZLIB_DIR=${ZLIB_DIR}")
message(STATUS "ZLIB_INCLUDE_DIR=${ZLIB_INCLUDE_DIR}")
message(STATUS "GTest_VERSION=${GTest_VERSION} GTest_DIR=${GTest_DIR}")

add_executable(app main.cpp)
target_link_libraries(app PRIVATE ZLIB::ZLIB GTest::gtest)
