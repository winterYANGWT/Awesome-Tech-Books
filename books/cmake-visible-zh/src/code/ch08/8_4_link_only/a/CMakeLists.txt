add_library(a STATIC a.cpp)
target_include_directories(a PUBLIC include)
target_link_libraries(a PRIVATE b)
