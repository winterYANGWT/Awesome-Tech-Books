include(GNUInstallDirs)
include(CMakePackageConfigHelpers)

set(RING_CMAKE_DIR "${CMAKE_INSTALL_LIBDIR}/cmake/ring")

install(TARGETS ring EXPORT ringTargets
  FILE_SET HEADERS DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
  ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})

install(EXPORT ringTargets
  FILE ringTargets.cmake
  NAMESPACE ring::
  DESTINATION ${RING_CMAKE_DIR})

configure_package_config_file(cmake/ringConfig.cmake.in
  "${CMAKE_CURRENT_BINARY_DIR}/ringConfig.cmake"
  INSTALL_DESTINATION ${RING_CMAKE_DIR})
write_basic_package_version_file(
  "${CMAKE_CURRENT_BINARY_DIR}/ringConfigVersion.cmake"
  VERSION ${PROJECT_VERSION}
  COMPATIBILITY SameMajorVersion)

install(FILES
  "${CMAKE_CURRENT_BINARY_DIR}/ringConfig.cmake"
  "${CMAKE_CURRENT_BINARY_DIR}/ringConfigVersion.cmake"
  DESTINATION ${RING_CMAKE_DIR})
