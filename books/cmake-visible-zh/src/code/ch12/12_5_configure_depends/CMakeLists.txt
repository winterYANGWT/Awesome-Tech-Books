cmake_minimum_required(VERSION 3.25...4.2)
project(ConfigureDepends LANGUAGES CXX)

option(TRACK_SETTINGS "把 settings.txt 登记为配置依赖" OFF)

file(READ ${CMAKE_CURRENT_SOURCE_DIR}/settings.txt SETTINGS)
string(STRIP "${SETTINGS}" SETTINGS)
configure_file(settings.h.in ${CMAKE_CURRENT_BINARY_DIR}/settings.h)
configure_file(template.h.in ${CMAKE_CURRENT_BINARY_DIR}/template.h)

if(TRACK_SETTINGS)
  set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS
               ${CMAKE_CURRENT_SOURCE_DIR}/settings.txt)
  get_property(configure_depends DIRECTORY PROPERTY CMAKE_CONFIGURE_DEPENDS)
  message(STATUS "配置依赖：${configure_depends}")
endif()

add_executable(app main.cpp)
target_include_directories(app PRIVATE ${CMAKE_CURRENT_BINARY_DIR})
