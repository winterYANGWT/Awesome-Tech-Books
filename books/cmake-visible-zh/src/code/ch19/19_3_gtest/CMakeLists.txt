cmake_minimum_required(VERSION 3.25...4.2)
project(GTestDemo LANGUAGES CXX)

find_package(GTest CONFIG REQUIRED)
include(CTest)
include(GoogleTest)

add_executable(t test.cpp)
target_link_libraries(t PRIVATE GTest::gtest_main)
gtest_discover_tests(t)
