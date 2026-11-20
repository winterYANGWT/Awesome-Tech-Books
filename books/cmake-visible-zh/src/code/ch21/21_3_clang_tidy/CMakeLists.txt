cmake_minimum_required(VERSION 3.25...4.2)
project(Tidy LANGUAGES CXX)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_CXX_CLANG_TIDY clang-tidy -checks=-*,readability-braces-around-statements)

add_library(core STATIC core.cpp)
target_compile_features(core PUBLIC cxx_std_17)

add_executable(app main.cpp)
target_link_libraries(app PRIVATE core)

# 目标属性优先于变量的默认值：app 不做这项检查
set_target_properties(app PROPERTIES CXX_CLANG_TIDY "")
