cmake_minimum_required(VERSION 3.25...4.2)
project(PathDemo LANGUAGES CXX)

set(SRC_FILE "${CMAKE_CURRENT_SOURCE_DIR}/src/widget.cpp")

cmake_path(GET SRC_FILE FILENAME file_name)
cmake_path(GET SRC_FILE EXTENSION file_ext)
cmake_path(GET SRC_FILE STEM file_stem)
cmake_path(GET SRC_FILE PARENT_PATH file_dir)
cmake_path(RELATIVE_PATH file_dir BASE_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}" OUTPUT_VARIABLE file_rel)
cmake_path(IS_ABSOLUTE SRC_FILE file_abs)
cmake_path(APPEND out_dir "${CMAKE_BINARY_DIR}" bin tools)

message(STATUS "FILENAME=${file_name}  EXTENSION=${file_ext}  STEM=${file_stem}")
message(STATUS "PARENT=${file_dir}")
message(STATUS "相对源码根=${file_rel}  绝对路径=${file_abs}")
message(STATUS "输出目录=${out_dir}")

get_filename_component(old_name "${SRC_FILE}" NAME)
get_filename_component(old_ext "${SRC_FILE}" EXT)
get_filename_component(old_stem "${SRC_FILE}" NAME_WE)
get_filename_component(old_dir "${SRC_FILE}" DIRECTORY)
message(STATUS "旧写法：NAME=${old_name}  EXT=${old_ext}  STEM=${old_stem}  DIRECTORY=${old_dir}")

add_executable(widget main.cpp "${SRC_FILE}")
target_include_directories(widget PRIVATE "${file_dir}")
set_target_properties(widget PROPERTIES
  OUTPUT_NAME "${file_stem}"
  RUNTIME_OUTPUT_DIRECTORY "${out_dir}")
