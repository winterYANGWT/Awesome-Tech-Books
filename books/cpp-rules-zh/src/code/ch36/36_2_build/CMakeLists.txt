cmake_minimum_required(VERSION 3.20)
project(freight LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_library(freight src/freight.cpp)
target_include_directories(freight PUBLIC include)
target_compile_options(freight PRIVATE -Wall -Wextra -Wpedantic -Werror)

add_executable(freight_test tests/freight_test.cpp)
target_link_libraries(freight_test PRIVATE freight)
target_compile_options(freight_test PRIVATE -Wall -Wextra -Wpedantic -Werror)

enable_testing()
add_test(NAME freight_test COMMAND freight_test)
