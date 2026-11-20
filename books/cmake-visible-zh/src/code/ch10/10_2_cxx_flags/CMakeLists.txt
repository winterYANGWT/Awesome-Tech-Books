cmake_minimum_required(VERSION 3.25...4.2)
project(FlagScope LANGUAGES CXX)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -DFROM_TOP_FLAGS")

add_subdirectory(third_party)

add_executable(app main.cpp)
target_compile_options(app PRIVATE -Wextra)
target_link_libraries(app PRIVATE tiny)
