cmake_minimum_required(VERSION 3.25...4.2)
project(ListOps LANGUAGES CXX)

set(SOURCES core net log gui_win gui_linux)
list(APPEND SOURCES core)                 # 故意重复一个
list(REMOVE_DUPLICATES SOURCES)

if(WIN32)
  list(FILTER SOURCES EXCLUDE REGEX "^gui_linux$")
else()
  list(FILTER SOURCES EXCLUDE REGEX "^gui_win$")
endif()

list(TRANSFORM SOURCES PREPEND "src/")
list(TRANSFORM SOURCES REPLACE "$" ".cpp")
list(SORT SOURCES)

list(FIND SOURCES "src/net.cpp" NET_INDEX)
list(JOIN SOURCES ", " SOURCE_TEXT)
message(STATUS "源文件（net.cpp 的下标 ${NET_INDEX}）：${SOURCE_TEXT}")

add_library(mylib STATIC ${SOURCES})
target_include_directories(mylib PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/src")

add_executable(app main.cpp)
target_link_libraries(app PRIVATE mylib)
