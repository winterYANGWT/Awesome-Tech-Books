add_library(net STATIC net.cpp)
target_include_directories(net PUBLIC include)
target_compile_definitions(net PRIVATE NET_PRIVATE=1)
target_link_libraries(net PRIVATE core)
