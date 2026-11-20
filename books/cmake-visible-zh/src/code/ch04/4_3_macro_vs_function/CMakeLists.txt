cmake_minimum_required(VERSION 3.25...4.2)
project(MacroVsFunction LANGUAGES CXX)

macro(set_with_macro name value)
  set(${name} "${value}")
  set(MACRO_TEMP "宏写在调用者里")
  if(DEFINED name)
    message(STATUS "宏里 if(DEFINED name) 为真")
  else()
    message(STATUS "宏里 if(DEFINED name) 为假")
  endif()
endmacro()

function(set_with_function name value)
  set(${name} "${value}" PARENT_SCOPE)
  set(FUNC_TEMP "只在函数里")
  if(DEFINED name)
    message(STATUS "函数里 if(DEFINED name) 为真")
  endif()
endfunction()

set_with_macro(FROM_MACRO yes)
set_with_function(FROM_FUNCTION yes)

message(STATUS "调用者里 FROM_MACRO=${FROM_MACRO} FROM_FUNCTION=${FROM_FUNCTION}")
if(DEFINED MACRO_TEMP)
  message(STATUS "MACRO_TEMP 留在了调用者里：${MACRO_TEMP}")
endif()
if(DEFINED FUNC_TEMP)
  message(STATUS "FUNC_TEMP 还在")
else()
  message(STATUS "FUNC_TEMP 不在调用者里")
endif()

add_executable(app main.cpp)
target_compile_definitions(app PRIVATE
  "FROM_MACRO=\"${FROM_MACRO}\""
  "FROM_FUNCTION=\"${FROM_FUNCTION}\""
  "MACRO_TEMP=\"${MACRO_TEMP}\"")
