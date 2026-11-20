cmake_minimum_required(VERSION 3.25...4.2)
project(XmlVars LANGUAGES CXX)

find_package(LibXml2 REQUIRED)

add_executable(app main.cpp)
target_link_libraries(app PRIVATE LibXml2::LibXml2)
