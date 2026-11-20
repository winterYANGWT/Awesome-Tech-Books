cmake_minimum_required(VERSION 3.25...4.2)
project(Components LANGUAGES CXX)

list(APPEND CMAKE_PREFIX_PATH "${CMAKE_CURRENT_SOURCE_DIR}/deps/prefix")
find_package(Tiny 9.0 CONFIG REQUIRED)
