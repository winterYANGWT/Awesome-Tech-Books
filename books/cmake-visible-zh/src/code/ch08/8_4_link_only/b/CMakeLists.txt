add_library(b STATIC b.cpp)
target_include_directories(b PUBLIC include)
target_compile_definitions(b PRIVATE B_PRIVATE=1)
