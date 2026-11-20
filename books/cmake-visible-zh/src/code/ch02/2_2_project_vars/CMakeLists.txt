cmake_minimum_required(VERSION 3.25...4.2)
project(Demo VERSION 1.4.2 DESCRIPTION "Two project scopes" LANGUAGES CXX)

set(DEMO_TAG "${PROJECT_NAME}-${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}.${PROJECT_VERSION_PATCH}")
message(STATUS "top: name=${PROJECT_NAME}; version=${PROJECT_VERSION}; tag=${DEMO_TAG}; top=${PROJECT_IS_TOP_LEVEL}; cmake=${CMAKE_PROJECT_NAME}")

add_executable(app main.cpp)
target_compile_definitions(app PRIVATE DEMO_TAG="${DEMO_TAG}")
add_subdirectory(sub)
target_link_libraries(app PRIVATE sub)

message(STATUS "back: name=${PROJECT_NAME}; version=${PROJECT_VERSION}; top=${PROJECT_IS_TOP_LEVEL}")
