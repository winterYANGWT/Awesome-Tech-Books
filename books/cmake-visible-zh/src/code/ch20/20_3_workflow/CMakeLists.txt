cmake_minimum_required(VERSION 3.25...4.2)
project(Workflow LANGUAGES CXX)

add_library(greet greet.cpp)
target_include_directories(greet PUBLIC include)

add_executable(app main.cpp)
target_link_libraries(app PRIVATE greet)

enable_testing()
add_executable(test_greet test_greet.cpp)
target_link_libraries(test_greet PRIVATE greet)
add_test(NAME greet COMMAND test_greet)
