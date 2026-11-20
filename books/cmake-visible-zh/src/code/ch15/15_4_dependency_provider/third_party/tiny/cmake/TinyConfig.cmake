add_library(Tiny::tiny INTERFACE IMPORTED)
set_target_properties(Tiny::tiny PROPERTIES
  INTERFACE_INCLUDE_DIRECTORIES "${CMAKE_CURRENT_LIST_DIR}/../include")
set(Tiny_FOUND TRUE)
