message(STATUS "post_install.cmake：前缀是 ${CMAKE_INSTALL_PREFIX}")
file(APPEND "${CMAKE_INSTALL_PREFIX}/share/misc/build-info.txt" "post_install.cmake 执行了\n")
