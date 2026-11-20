cmake_minimum_required(VERSION 3.25...4.2)
project(Shadow LANGUAGES CXX)

set(BAR normal)
set(BAR cached CACHE STRING "BAR 的缓存值")
message(STATUS "BAR=${BAR} $CACHE{BAR}=$CACHE{BAR}")

set(FEATURE OFF)
option(FEATURE "Use the feature" ON)
message(STATUS "FEATURE=${FEATURE} cache_has_feature=[$CACHE{FEATURE}]")

set(LEVEL quiet CACHE STRING "Log level")
set(LEVEL normal)
message(STATUS "LEVEL=${LEVEL}")
unset(LEVEL)
message(STATUS "unset 后 LEVEL=${LEVEL}")

add_executable(app main.cpp)
target_compile_definitions(app PRIVATE
  BAR="${BAR}"
  CACHED_BAR="$CACHE{BAR}"
  FEATURE=$<BOOL:${FEATURE}>
  LEVEL="${LEVEL}")
