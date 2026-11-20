cmake_minimum_required(VERSION 3.27...4.2)
project(GenexKinds LANGUAGES CXX)

add_executable(app main.cpp)
set_target_properties(app PROPERTIES OUTPUT_NAME "genex-app")
target_compile_definitions(app PRIVATE "APP_CONFIG=\"$<CONFIG>\"")

file(GENERATE OUTPUT "${CMAKE_BINARY_DIR}/genex.txt"
  CONTENT "config=$<CONFIG>
if_debug=$<IF:$<CONFIG:Debug>,yes,no>
branch=$<$<CONFIG:Debug>:debug-only>
compiler_id=$<CXX_COMPILER_ID>
is_gnu_or_clang=$<CXX_COMPILER_ID:GNU,Clang>
lang_and_id=$<COMPILE_LANG_AND_ID:CXX,GNU>
target_file=$<TARGET_FILE:app>
target_output=$<TARGET_PROPERTY:app,OUTPUT_NAME>
list_join=$<JOIN:alpha;beta;gamma,|>
list_length=$<LIST:LENGTH,alpha;beta;gamma>
list_upper=$<LIST:TRANSFORM,alpha;beta,TOUPPER>
"
  TARGET app)
