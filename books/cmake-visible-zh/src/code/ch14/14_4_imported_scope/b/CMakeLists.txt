if(NOT TARGET Tiny::tiny)
  message(FATAL_ERROR "b 看不到 Tiny::tiny：它只在 a/ 及其子目录里可见")
endif()

get_target_property(tiny_includes Tiny::tiny INTERFACE_INCLUDE_DIRECTORIES)
message(STATUS "b 看到 Tiny::tiny，include=${tiny_includes}")

add_executable(b b.cpp)
target_link_libraries(b PRIVATE Tiny::tiny)
