message(STATUS "provider: 在 project() 里被 include")
set(TINY_LOCAL_DIR "${CMAKE_CURRENT_LIST_DIR}/../third_party/tiny"
  CACHE PATH "本地的 Tiny 目录")

macro(tiny_provide_dependency method dep_name)
  if("${method}" STREQUAL "FIND_PACKAGE" AND "${dep_name}" STREQUAL "Tiny")
    set(Tiny_DIR "${TINY_LOCAL_DIR}/cmake" CACHE PATH "Tiny 包的位置")
    find_package(Tiny CONFIG REQUIRED BYPASS_PROVIDER)
    set(Tiny_FOUND TRUE)
  endif()
endmacro()

cmake_language(SET_DEPENDENCY_PROVIDER tiny_provide_dependency
  SUPPORTED_METHODS FIND_PACKAGE)
