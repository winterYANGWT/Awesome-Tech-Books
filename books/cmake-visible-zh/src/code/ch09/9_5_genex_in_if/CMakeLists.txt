cmake_minimum_required(VERSION 3.25...4.2)
project(GenexInIf LANGUAGES CXX)

add_executable(app main.cpp)

# 反例：if() 不会求值生成器表达式
if("$<CONFIG>" STREQUAL "Debug")
  message(STATUS "if() 选中了 Debug 分支")
  target_compile_definitions(app PRIVATE DEBUG_BRANCH)
else()
  message(STATUS "if() 没有选中；它看到的是字面量 [$<CONFIG>]")
endif()

# 调试办法之一：让构建阶段打印求值结果
add_custom_target(genexdebug
  COMMAND ${CMAKE_COMMAND} -E echo "生成时 $<CONFIG>")
