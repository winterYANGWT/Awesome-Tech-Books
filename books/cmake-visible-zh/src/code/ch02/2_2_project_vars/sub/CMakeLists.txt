project(Sub VERSION 0.3 LANGUAGES CXX)

set(SUB_TAG "${PROJECT_NAME}-${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}")
message(STATUS "sub: name=${PROJECT_NAME}; version=${PROJECT_VERSION}; tag=${SUB_TAG}; top=${PROJECT_IS_TOP_LEVEL}; sub_top=${Sub_IS_TOP_LEVEL}; cmake=${CMAKE_PROJECT_NAME}")

add_library(sub STATIC sub.cpp)
target_include_directories(sub PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")
target_compile_definitions(sub PRIVATE SUB_TAG="${SUB_TAG}")
