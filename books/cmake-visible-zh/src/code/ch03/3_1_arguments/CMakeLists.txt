cmake_minimum_required(VERSION 3.25...4.2)
project(Arguments LANGUAGES CXX)

set(EMPTY "")

# show() 只是把收到的参数原样报出来，再把 ARGC 记进一个全局属性
function(show)
  message(STATUS "ARGC=${ARGC}")
  set(i 0)
  while(i LESS ARGC)
    message(STATUS "  ARGV${i}=[${ARGV${i}}]")
    math(EXPR i "${i} + 1")
  endwhile()
  set_property(GLOBAL APPEND PROPERTY argc_log "ARGC=${ARGC}")
endfunction()

show(a b c)
show("a b c")
show(a;b;c)
show("a;b;c")
show(${EMPTY})
show("${EMPTY}")
show("" "")
show(one "two three" four;five)

get_property(counts GLOBAL PROPERTY argc_log)
string(REPLACE "ARGC=" "" counts_text "${counts}")
string(REPLACE ";" "," counts_text "${counts_text}")

add_executable(app main.cpp)
target_compile_definitions(app PRIVATE "ARG_COUNTS=\"${counts_text}\"")
