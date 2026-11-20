cmake_minimum_required(VERSION 3.25...4.2)
project(InstallDirs LANGUAGES CXX)

include(GNUInstallDirs)
message(STATUS "安装目录：bin=${CMAKE_INSTALL_BINDIR} lib=${CMAKE_INSTALL_LIBDIR} include=${CMAKE_INSTALL_INCLUDEDIR}")

add_library(dirs STATIC dirs.cpp)
target_include_directories(dirs PUBLIC include)

add_executable(dirs_app main.cpp)
target_link_libraries(dirs_app PRIVATE dirs)

install(TARGETS dirs dirs_app
  ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
install(FILES include/dirs/dirs.h DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/dirs)
