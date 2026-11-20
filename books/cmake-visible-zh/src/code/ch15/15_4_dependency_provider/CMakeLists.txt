cmake_minimum_required(VERSION 3.25...4.2)
project(ProviderDemo LANGUAGES CXX)

find_package(Tiny CONFIG REQUIRED)
message(STATUS "Tiny_FOUND=${Tiny_FOUND} Tiny_DIR=${Tiny_DIR}")

add_executable(app main.cpp)
target_link_libraries(app PRIVATE Tiny::tiny)
