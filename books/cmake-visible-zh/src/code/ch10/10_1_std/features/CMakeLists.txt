add_library(std_features std_features.cpp)
target_compile_features(std_features PUBLIC cxx_std_17)

add_executable(app main.cpp)
target_link_libraries(app PRIVATE std_features)

file(GENERATE OUTPUT "${CMAKE_BINARY_DIR}/features.txt"
  CONTENT "std_features=$<TARGET_PROPERTY:std_features,COMPILE_FEATURES>\napp=$<TARGET_PROPERTY:app,COMPILE_FEATURES>\n")
