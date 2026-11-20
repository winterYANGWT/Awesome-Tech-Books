add_executable(calc_test calc_test.cpp)
target_link_libraries(calc_test PRIVATE calc)
add_test(NAME calc_add COMMAND calc_test)
