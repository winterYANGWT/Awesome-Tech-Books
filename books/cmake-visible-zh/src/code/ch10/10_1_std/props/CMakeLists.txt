add_library(std_props std_props.cpp)
set_target_properties(std_props PROPERTIES
  CXX_STANDARD 14
  CXX_STANDARD_REQUIRED ON
  CXX_EXTENSIONS OFF)
