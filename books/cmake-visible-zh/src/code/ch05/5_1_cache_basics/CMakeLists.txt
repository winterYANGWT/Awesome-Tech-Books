cmake_minimum_required(VERSION 3.25...4.2)
project(CacheBasics LANGUAGES CXX)

option(USE_FAST "Use the fast code path" ON)
set(APP_MODE "demo" CACHE STRING "Which mode the app runs in")
set(APP_DATA_DIR "${CMAKE_SOURCE_DIR}/data" CACHE PATH "Where the data lives")
set(ENABLE_LOG OFF CACHE BOOL "Write a log line at startup")
set(BUILD_TOKEN "internal-42" CACHE INTERNAL "Build token, not for users")

message(STATUS "USE_FAST=${USE_FAST} mode=${APP_MODE} data=${APP_DATA_DIR}")
message(STATUS "log=${ENABLE_LOG} token=${BUILD_TOKEN}")

add_executable(app main.cpp)
if(USE_FAST)
  target_compile_definitions(app PRIVATE FAST_PATH=1)
else()
  target_compile_definitions(app PRIVATE FAST_PATH=0)
endif()
target_compile_definitions(app PRIVATE
  APP_MODE="${APP_MODE}"
  APP_DATA_DIR="${APP_DATA_DIR}"
  LOG_ENABLED=$<BOOL:${ENABLE_LOG}>)
