cmake_minimum_required(VERSION 3.25...4.2)
project(FlagScope LANGUAGES CXX)

add_subdirectory(third_party)

add_executable(app main.cpp)
target_compile_options(app PRIVATE -Wall -Wextra -DFROM_TOP_FLAGS)
target_link_libraries(app PRIVATE tiny)
