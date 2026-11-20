include(FetchContent)
FetchContent_Declare(googletest
  URL https://github.com/google/googletest/releases/download/v1.17.0/googletest-1.17.0.tar.gz
  FIND_PACKAGE_ARGS NAMES GTest CONFIG)
FetchContent_MakeAvailable(googletest)

add_executable(ring_test ring_test.cpp)
target_link_libraries(ring_test PRIVATE ring::ring GTest::gtest_main)

include(GoogleTest)
gtest_discover_tests(ring_test)
