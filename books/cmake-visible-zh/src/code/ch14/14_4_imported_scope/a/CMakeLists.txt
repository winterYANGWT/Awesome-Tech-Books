find_package(Tiny CONFIG REQUIRED)

add_library(a STATIC a.cpp)
target_link_libraries(a PUBLIC Tiny::tiny)
