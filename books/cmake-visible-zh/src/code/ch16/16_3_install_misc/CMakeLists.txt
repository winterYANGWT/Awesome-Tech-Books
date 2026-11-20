cmake_minimum_required(VERSION 3.25...4.2)
project(InstallMisc LANGUAGES CXX)

add_executable(misc_tool main.cpp)

install(TARGETS misc_tool RUNTIME DESTINATION bin)
install(FILES README.md DESTINATION share/doc/misc)
install(DIRECTORY assets/ DESTINATION share/misc/assets)

install(CODE [[
  message(STATUS "install(CODE)：写 build-info.txt")
  file(WRITE "${CMAKE_INSTALL_PREFIX}/share/misc/build-info.txt" "misc 1.0\n")
]])
install(SCRIPT cmake/post_install.cmake)
