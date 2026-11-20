cmake_minimum_required(VERSION 3.25...4.2)
project(Lists LANGUAGES CXX)

set(L a b c)
message(STATUS "L=[${L}]")
list(APPEND L d)
list(LENGTH L len)
list(GET L 0 first)
list(JOIN L " " joined)
message(STATUS "len=${len} first=${first} joined=[${joined}]")

set(FLAGS -Wall -Wextra)
list(LENGTH FLAGS n_flags)
set(SPACED "-Wall -Wextra")
list(LENGTH SPACED n_spaced)
message(STATUS "FLAGS len=${n_flags}; SPACED len=${n_spaced} value=[${SPACED}]")

add_executable(app main.cpp)
target_compile_options(app PRIVATE ${FLAGS})
add_executable(app_quoted main.cpp)
target_compile_options(app_quoted PRIVATE "${FLAGS}")
get_target_property(opts_a app COMPILE_OPTIONS)
get_target_property(opts_q app_quoted COMPILE_OPTIONS)
message(STATUS "app=[${opts_a}] app_quoted=[${opts_q}]")

add_custom_target(show_unquoted
  COMMAND ${CMAKE_COMMAND} -E echo ${FLAGS} VERBATIM)
add_custom_target(show_quoted
  COMMAND ${CMAKE_COMMAND} -E echo "${FLAGS}" VERBATIM)
