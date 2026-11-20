cmake_minimum_required(VERSION 3.25...4.2)
project(ImportedScope LANGUAGES CXX)

list(APPEND CMAKE_PREFIX_PATH "${CMAKE_CURRENT_SOURCE_DIR}/deps/prefix")
add_subdirectory(a)
add_subdirectory(b)
