cmake_minimum_required(VERSION 3.25...4.2)
project(Checks LANGUAGES CXX)

include(CheckCXXSourceCompiles)
include(CheckIncludeFileCXX)
include(CheckCXXCompilerFlag)

# 只在第二次配置时执行：第一次还没有这个缓存项
if(DEFINED HAVE_CXX_STRING)
  message(STATUS "再配置：HAVE_CXX_STRING 已在缓存里")
endif()

check_cxx_source_compiles("
#include <string>
int main() { std::string s = \"checks\"; return s.size() == 6 ? 0 : 1; }
" HAVE_CXX_STRING)

check_include_file_cxx(unistd.h HAVE_UNISTD_H)
check_cxx_compiler_flag(-fno-exceptions HAVE_FNO_EXCEPTIONS)
check_cxx_compiler_flag(-fno-such-flag HAVE_BOGUS_FLAG)

message(STATUS "结果：string=${HAVE_CXX_STRING} unistd=${HAVE_UNISTD_H} no-exceptions=${HAVE_FNO_EXCEPTIONS} bogus=${HAVE_BOGUS_FLAG}")

add_executable(app main.cpp)
if(HAVE_UNISTD_H)
  target_compile_definitions(app PRIVATE HAVE_UNISTD_H=1)
endif()
if(HAVE_FNO_EXCEPTIONS)
  target_compile_options(app PRIVATE -fno-exceptions)
endif()
