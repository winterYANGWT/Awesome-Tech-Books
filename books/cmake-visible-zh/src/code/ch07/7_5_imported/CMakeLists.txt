cmake_minimum_required(VERSION 3.25...4.2)
project(Imported LANGUAGES CXX)

find_package(ZLIB REQUIRED)

add_executable(app main.cpp)
target_link_libraries(app PRIVATE ZLIB::ZLIB)
