cmake_minimum_required(VERSION 3.25...4.2)
project(GlobSources LANGUAGES CXX)

file(GLOB SOURCES CONFIGURE_DEPENDS "*.cpp")

add_executable(app ${SOURCES})
target_include_directories(app PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
