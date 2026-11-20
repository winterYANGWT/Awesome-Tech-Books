add_library(core STATIC core.cpp)
target_include_directories(core PUBLIC include)
target_compile_definitions(core PRIVATE CORE_PRIVATE=1)
