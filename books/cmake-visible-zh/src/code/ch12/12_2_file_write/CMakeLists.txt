cmake_minimum_required(VERSION 3.25...4.2)
project(FileWrite LANGUAGES CXX)

set(APP_VERSION "1.2.3")

file(WRITE ${CMAKE_CURRENT_BINARY_DIR}/written.h  # 每次配置都覆盖
     "#pragma once\n#define WRITTEN_VERSION \"${APP_VERSION}\"\n")

file(APPEND ${CMAKE_CURRENT_BINARY_DIR}/configure.log "configured\n")  # 每次追加

file(CONFIGURE OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/configured.h  # 内容不变就不写
     CONTENT "#pragma once\n#define CONFIGURED_VERSION \"@APP_VERSION@\"\n")

file(GENERATE OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/generated.h  # 生成阶段写，内容不变不写
     CONTENT "#pragma once\n#define GENERATED_PLATFORM \"$<PLATFORM_ID>\"\n")

configure_file(version.h.in ${CMAKE_CURRENT_BINARY_DIR}/version.h)

add_executable(write_app written.cpp)
add_executable(conf_app configured.cpp)
add_executable(gen_app generated.cpp)
target_include_directories(write_app PRIVATE ${CMAKE_CURRENT_BINARY_DIR})
target_include_directories(conf_app PRIVATE ${CMAKE_CURRENT_BINARY_DIR})
target_include_directories(gen_app PRIVATE ${CMAKE_CURRENT_BINARY_DIR})
