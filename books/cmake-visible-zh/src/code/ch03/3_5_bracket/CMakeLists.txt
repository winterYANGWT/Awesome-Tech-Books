cmake_minimum_required(VERSION 3.25...4.2)
project(Bracket LANGUAGES CXX)

#[[ 块注释可以从这里
一直写到这里；里面的 ${VAR}、"引号" 都不会执行。 ]]

set(VERSION_TEXT "v1.4.2")
set(RE [=[^v([0-9]+)\.([0-9]+)\.([0-9]+)$]=])
if(VERSION_TEXT MATCHES "${RE}")
  set(MAJOR "${CMAKE_MATCH_1}")
  set(MINOR "${CMAKE_MATCH_2}")
  set(PATCH "${CMAKE_MATCH_3}")
endif()
message(STATUS "版本 ${MAJOR}.${MINOR}.${PATCH}，\
来自 ${VERSION_TEXT}")

set(META [=[
{
  "name": "app",
  "version": "1.4.2",
  "note": "${这里不会展开}"
}
]=])
file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/meta.json" "${META}")
message(STATUS [=[meta 的 note 原样保留：${这里不会展开}]=])

add_executable(app main.cpp)
target_compile_definitions(app PRIVATE "APP_VERSION=\"${MAJOR}.${MINOR}.${PATCH}\"")
