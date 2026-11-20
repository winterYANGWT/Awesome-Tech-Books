get_filename_component(_Tiny_prefix "${CMAKE_CURRENT_LIST_DIR}/../../.." ABSOLUTE)

add_library(Tiny::tiny INTERFACE IMPORTED)
set_target_properties(Tiny::tiny PROPERTIES
  INTERFACE_INCLUDE_DIRECTORIES "${_Tiny_prefix}/include")
