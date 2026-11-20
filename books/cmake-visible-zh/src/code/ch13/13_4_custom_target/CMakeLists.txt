cmake_minimum_required(VERSION 3.25...4.2)
project(CustomTarget LANGUAGES CXX)

add_executable(app main.cpp)

add_custom_target(stamp
  COMMAND ${CMAKE_COMMAND} -E touch ${CMAKE_CURRENT_BINARY_DIR}/stamp.txt
  COMMENT "stamp: 写一次时间戳"
  VERBATIM)
add_dependencies(app stamp)

add_custom_target(audit ALL
  COMMAND ${CMAKE_COMMAND} -E echo "audit: 每次默认构建都执行"
  VERBATIM)

add_custom_target(format
  COMMAND clang-format --dry-run --Werror main.cpp
  WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
  COMMENT "检查 main.cpp 的格式"
  VERBATIM)
