cmake_minimum_required(VERSION 3.25...4.2)
project(When LANGUAGES CXX)

# 配置阶段：这几行现在就执行
message(STATUS "C++ 编译器：${CMAKE_CXX_COMPILER}")
file(WRITE "${CMAKE_BINARY_DIR}/configured.txt" "配置阶段写的；编译器 ${CMAKE_CXX_COMPILER_ID}\n")

# 生成阶段才求值：$<...> 现在还只是一段字符串
file(GENERATE OUTPUT "${CMAKE_BINARY_DIR}/generated.txt"
     CONTENT "生成阶段写的；app 的文件名是 $<TARGET_FILE_NAME:app>\n")

add_executable(app main.cpp)

# 构建阶段：链接完 app 之后才执行
add_custom_command(TARGET app POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E echo "构建阶段：$<TARGET_FILE_NAME:app> 链接好了")
