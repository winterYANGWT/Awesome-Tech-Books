add_test(NAME package_install COMMAND
  ${CMAKE_COMMAND} --install ${CMAKE_BINARY_DIR}
  --prefix ${CMAKE_BINARY_DIR}/prefix)
set_tests_properties(package_install PROPERTIES
  FIXTURES_SETUP package_prefix)

add_test(NAME package_consumer COMMAND
  ${CMAKE_COMMAND} -S ${CMAKE_SOURCE_DIR}/app
  -B ${CMAKE_BINARY_DIR}/consumer -G Ninja
  -DCMAKE_PREFIX_PATH=${CMAKE_BINARY_DIR}/prefix)
set_tests_properties(package_consumer PROPERTIES
  FIXTURES_REQUIRED package_prefix
  FIXTURES_SETUP consumer_ready)

add_test(NAME package_consumer_build COMMAND
  ${CMAKE_COMMAND} --build ${CMAKE_BINARY_DIR}/consumer --target run)
set_tests_properties(package_consumer_build PROPERTIES
  FIXTURES_REQUIRED consumer_ready
  PASS_REGULAR_EXPRESSION "hello, installed package!")
