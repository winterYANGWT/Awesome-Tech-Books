cmake_minimum_required(VERSION 3.25...4.2)
project(InstallParts LANGUAGES CXX)

include(GNUInstallDirs)

add_library(parts STATIC parts.cpp)
target_include_directories(parts PUBLIC include)

add_executable(parts_app main.cpp)
target_link_libraries(parts_app PRIVATE parts)

install(TARGETS parts_app
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR} COMPONENT runtime)
install(TARGETS parts
  ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR} COMPONENT devel)
install(FILES include/parts/parts.h
  DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/parts COMPONENT devel)
