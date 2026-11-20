cmake_minimum_required(VERSION 3.25...4.2)
project(Inherits LANGUAGES CXX)

message(STATUS "sanitize=${PROJ_SANITIZE} werror=${PROJ_WERROR} label=$ENV{PROJ_LABEL}")

set(PROJ_LABEL "$ENV{PROJ_LABEL}")
set(PROJ_SANITIZE_TEXT "${PROJ_SANITIZE}")
configure_file(preset_info.h.in preset_info.h @ONLY)

add_executable(app main.cpp)
target_include_directories(app PRIVATE "${CMAKE_CURRENT_BINARY_DIR}")

if(PROJ_WARNINGS)
  target_compile_options(app PRIVATE -Wall -Wextra)
endif()
if(PROJ_WERROR)
  target_compile_options(app PRIVATE -Werror)
endif()
if(PROJ_SANITIZE)
  string(REPLACE ";" "," PROJ_SANITIZE_LIST "${PROJ_SANITIZE}")
  target_compile_options(app PRIVATE -fsanitize=${PROJ_SANITIZE_LIST} -fno-omit-frame-pointer)
  target_link_options(app PRIVATE -fsanitize=${PROJ_SANITIZE_LIST})
endif()
