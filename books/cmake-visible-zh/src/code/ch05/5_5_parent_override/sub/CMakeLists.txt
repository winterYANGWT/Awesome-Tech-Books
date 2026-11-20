option(SUB_BUILD_TESTS "Build sub's tests" ON)
message(STATUS "sub 里 SUB_BUILD_TESTS=${SUB_BUILD_TESTS}")

add_library(sub STATIC sub.cpp)

if(SUB_BUILD_TESTS)
  add_executable(sub_test test.cpp)
  target_link_libraries(sub_test PRIVATE sub)
endif()
