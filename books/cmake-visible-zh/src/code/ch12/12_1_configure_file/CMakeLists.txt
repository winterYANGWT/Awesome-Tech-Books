cmake_minimum_required(VERSION 3.25...4.2)
project(ConfigureFile VERSION 1.2.3 LANGUAGES CXX)

set(APP_NAME "config-demo")
set(ENABLE_LOGGING ON)
set(HAVE_OPENSSL OFF)

configure_file(config.h.in ${CMAKE_CURRENT_BINARY_DIR}/config.h)

add_executable(app main.cpp)
target_include_directories(app PRIVATE ${CMAKE_CURRENT_BINARY_DIR})
