cmake_minimum_required(VERSION 3.25...4.2)
project(LinkOnly LANGUAGES CXX)

add_subdirectory(b)
add_subdirectory(a)
add_subdirectory(app)
