cmake_minimum_required(VERSION 3.25...4.2)
project(Vendored LANGUAGES CXX)

add_subdirectory(third_party/tiny EXCLUDE_FROM_ALL SYSTEM)

add_executable(app main.cpp)
target_link_libraries(app PRIVATE tiny)
