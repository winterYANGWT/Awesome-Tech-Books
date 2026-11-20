cmake_minimum_required(VERSION 3.25...4.2)
project(InitOrder LANGUAGES CXX)

add_executable(before before.cpp)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)
set(CMAKE_CXX_VISIBILITY_PRESET hidden)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")

add_executable(after after.cpp)

get_target_property(before_std before CXX_STANDARD)
get_target_property(after_std after CXX_STANDARD)
message(STATUS "before=${before_std} after=${after_std}")
