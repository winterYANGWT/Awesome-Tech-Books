cmake_minimum_required(VERSION 3.25...4.2)
project(TestProps LANGUAGES CXX)

enable_testing()

add_executable(probe probe.cpp)

add_test(NAME wrong_exit COMMAND probe fail)
set_tests_properties(wrong_exit PROPERTIES WILL_FAIL TRUE)

add_test(NAME greeting COMMAND probe greet)
set_tests_properties(greeting PROPERTIES
  PASS_REGULAR_EXPRESSION "hello, CMake"
  ENVIRONMENT "GREETING=CMake"
  LABELS "unit;fast")

add_test(NAME slow_probe COMMAND probe sleep)
set_tests_properties(slow_probe PROPERTIES TIMEOUT 1 LABELS "slow")

add_test(NAME fixture_setup COMMAND probe setup)
set_tests_properties(fixture_setup PROPERTIES FIXTURES_SETUP DATA)

add_test(NAME fixture_user COMMAND probe check)
set_tests_properties(fixture_user PROPERTIES FIXTURES_REQUIRED DATA LABELS "unit")

add_test(NAME fixture_cleanup COMMAND probe cleanup)
set_tests_properties(fixture_cleanup PROPERTIES FIXTURES_CLEANUP DATA)
