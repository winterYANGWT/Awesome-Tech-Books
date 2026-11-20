cmake_minimum_required(VERSION 3.25...4.2)
project(AddTest LANGUAGES CXX)

enable_testing()

add_executable(probe probe.cpp)

add_test(NAME returns_zero COMMAND probe ok)
add_test(NAME returns_one COMMAND probe fail)
