cmake_minimum_required(VERSION 3.25...4.2)
project(InstallRpath LANGUAGES CXX)

include(GNUInstallDirs)

add_library(greet SHARED greet.cpp)
target_include_directories(greet PUBLIC include)

# CMAKE_INSTALL_RPATH 只初始化之后创建的目标；这里放在共享库之后、程序之前
option(WITH_INSTALL_RPATH "给安装后的程序加 RUNPATH" OFF)
if(WITH_INSTALL_RPATH)
  set(CMAKE_INSTALL_RPATH "$ORIGIN/../${CMAKE_INSTALL_LIBDIR}")
endif()

add_executable(greet_app main.cpp)
target_link_libraries(greet_app PRIVATE greet)

install(TARGETS greet LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR})
install(TARGETS greet_app RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
