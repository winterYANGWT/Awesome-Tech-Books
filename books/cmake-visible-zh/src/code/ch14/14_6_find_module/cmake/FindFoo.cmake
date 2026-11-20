set(_Foo_root "${CMAKE_CURRENT_LIST_DIR}/../deps/foo")
# 示例的假库在 deps/foo（libfoo.a 由 src/foo.cpp 编成）；NO_DEFAULT_PATH 让 find_* 只查这里

find_path(Foo_INCLUDE_DIR NAMES foo/foo.h
  PATHS "${_Foo_root}/include" NO_DEFAULT_PATH)
find_library(Foo_LIBRARY NAMES foo
  PATHS "${_Foo_root}/lib" NO_DEFAULT_PATH)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(Foo REQUIRED_VARS Foo_LIBRARY Foo_INCLUDE_DIR)

if(Foo_FOUND AND NOT TARGET Foo::Foo)
  add_library(Foo::Foo UNKNOWN IMPORTED)
  set_target_properties(Foo::Foo PROPERTIES
    IMPORTED_LOCATION "${Foo_LIBRARY}"
    INTERFACE_INCLUDE_DIRECTORIES "${Foo_INCLUDE_DIR}")
endif()
